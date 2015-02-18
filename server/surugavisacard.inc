<?php
/*
surugavisacard.inc: スルガ銀行 VISAデビット/VISAクレジットのHTMLよりOFXファイルを生成する
Copyright (C) 2012-2015 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["loginID"];
$pass1 = $ofxforms["telPin"];
$pass2 = $ofxforms["webPin"];
$mode = $ofxforms["mode"];

$resp = array();
$resp["status"] = ENV_NUM_STATUS_NONE;
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$urls = parse_url($settings["home"]);
$method = "GET";
$protocol = ($urls["scheme"] != ""? $urls["scheme"]: "");
$host = ($urls["host"] != ""? $urls["host"]: "");
$page = ($urls["path"] != ""? $urls["path"]: "/");
$query = "";
$cookie = "";
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

// ログイン画面を取得する
$protocol = 'https';
$page = '/Top.do';
$cookie = surugavisa_updatecookie($head, $cookie);
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "loginForm");
$method = "POST";
$urls = parse_url($forms[$c]['action']);
if($urls["scheme"] != "") $protocol = $urls["scheme"];
if($urls["host"] != "") $host = $urls["host"];
if($urls["path"] != "") $page = $urls["path"];
if($urls["query"] != "") $page .= "?" . $urls["query"];

$inputs = parse_tag($forms[$c]["innerHTML"], "input");
$queries = array();
foreach ($inputs as $input) {
    if ($input["name"] != "") {
        $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
    }
}
$queries["loginID"] .= $user;
$queries["telPin"] .= $pass1;
$queries["webPin"] .= $pass2;
$query = implode("&", $queries);
$cookie = surugavisa_updatecookie($head, $cookie);
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

if(strpos($body, "只今メンテナンス中です") !== false) {
    // システムメンテナンス画面の場合
    $resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "loginForm") !== false) {
    // ログイン失敗の場合
    $resp["status"] = ENV_NUM_STATUS_FAILURE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
} else {
    // カード名称・カード番号を取得する
    $account['acctname'] = 'スルガ Visaデビット/クレジット';
    $account['acctid'] = substr($user, 12);
    // DOMツリーを生成
    $ofxdom = new ofxDOM($account['acctid']);
    
    // メニューから「ご利用明細」を選択
    $cds = array();
    for ($i=0;$i<6;$i++) {
        $forms = parse_tag($body, "form");
        $c = parse_tag_search($forms, "name", "meisaiForm");
        $page = '/meisai.do';
        $inputs = parse_tag($forms[$c]["innerHTML"], "input");
        $queries = array();
        foreach ($inputs as $input) {
            if ($input["name"] != "") {
                $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
            }
        }
        $queries["month"] = "month=" . $i;
        $query = implode("&", $queries);
        $cookie = surugavisa_updatecookie($head, $cookie);
        list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
        // 明細をパース
        $cds = array_merge($cds, surugavisa_parsedom($body));
    }
    usort($cds, function($a, $b) {
        return $a['DTPOSTED'] > $b['DTPOSTED'];
    });
    $ofxdom->setTrans($cds);
    $cds_balamt = 0;
    $cd_num = 0;
    $cds_s = "";
    $cds_e = "";
    $items = $ofxdom->getTrans();
    foreach ($items as $item) {
        $dtposted = $item->getElementsByTagName('DTPOSTED')->item(0)->nodeValue;
        // トランザクション番号（後半）を生成する
        if ($dtposted == $cds_e) $cd_num++; else $cd_num = 0;
        $fitid = $item->getElementsByTagName('FITID')->item(0);
        $fitid->nodeValue .= sprintf("%05d", $cd_num);
        // DTSTART, DTENDを取得
        if($cds_s == "") $cds_s = $dtposted;
        $cds_e = $dtposted;
        // 残高を計算
        $cds_balamt += (double)$item->getElementsByTagName('TRNAMT')->item(0)->nodeValue;
    }
    
    // BANKTRANLIST
    $ofxdom->setBankTranList($cds_s, $cds_e);
    
    // 残高を処理
    $cds_balamt = (-1)*$cds_balamt;
    $ofxdom->setLedgerBalance($cds_balamt, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);
    
    //MKGTINFOを設定
    $ofxdom->setMktginfo(mb_convert_string($settings["name"]));
    
    // XML DOMツリーを文字列に変換
    $xml = $ofxdom->getXML();
    
    $resp["status"] = ENV_NUM_STATUS_SUCCESS;
    $resp["ofx"] = generate_ofx($mode, $xml);
    
    // 実行時間（タイムアウト）を再設定する
    @set_time_limit(ENV_NUM_TIMEOUT);
    
    // ログアウトする
    $forms = parse_tag($body, "form");
    $c = parse_tag_search($forms, "name", "logoutForm");
    $page = '/logout.do';
    $inputs = parse_tag($forms[$c]["innerHTML"], "input");
    $queries = array();
    foreach ($inputs as $input) {
        if ($input["name"] != "") {
            $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
        }
    }
    $query = implode("&", $queries);
    $cookie = surugavisa_updatecookie($head, $cookie);
    list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
}
return $resp;

// HTTP/1.1
function surugavisa_http11($method, $protocol, $host, $page = "/", $query = "", $cookie = "") {
    $ret = "INVALID HOST";
    if(preg_match("/card\.surugabank\.co\.jp$/", $host) > 0) $ret = http11($method, $protocol, $host, 0, $page, $query, "", $cookie);
    if(preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $ret, $matches) > 0) {
        $urls = parse_url($matches[1]);    
        if($urls["scheme"] != "") $protocol = $urls["scheme"];
        if($urls["host"] != "") $host = $urls["host"];
        if($urls["path"] != "") $page = $urls["path"];
        $query = ($urls["query"] != ""? $urls["query"]: "");
        return surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
    }
    return explode("\r\n\r\n", $ret, 2);
}

function surugavisa_updatecookie($head, $cookie) {
    return update_cookie(array("JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function surugavisa_parsedom($str) {
    // 明細表読み込み用DOMツリー作成
    $doc = new DOMDocument();
    // <meta … charset=Shift_JIS">の前の<title>が，文字化けの原因となるため，削除
    preg_match("/(<body.*<\/body>)/s", $str, $matches);
    $str = $matches[1];
    // ヘッダ付加   
    $str = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><\/head>" . $str . "<\/html>";
    // 文字エンコード変換
    $str = mb_convert_encoding($str, 'UTF-8', 'SJIS');
    // &nbsp;
    $str = str_replace("&nbsp;", "", $str);
    // HTMLからDOMツリー作成
    @$doc->loadHTML($str);
    $xpath = new DOMXPath($doc);
    // 明細表のテーブルを指定
    $rows = $xpath->query("//table[@class='table-type2 clear margin-b-10px']/tr"); 
    
    $ret = "";
    $cds = array();
    $nrow = $rows->length;
    
    for($i=0; $i<$nrow; $i++) {
        $cd = array();
        $row = $rows->item($i);
        $cols1 = $row->getElementsByTagName('th');
        $cols2 = $row->getElementsByTagName('td');
        // 利用明細でない行はスキップ
        if ($cols1->length != 2) continue;
        
        $cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CREDIT;
        
        // 日付を取得する
        preg_match("/([0-9]{4}).*?([0-9]{1,2}).*?([0-9]{1,2})/", str_replace(" ", "", $cols1->item(1)->nodeValue), $matches);
        $cd["DTPOSTED"] = sprintf("%d%02d%02d", $matches[1], $matches[2], $matches[3]);
        
        // トランザクション番号（前半）を生成する
        $cd["FITID"] = '000';
        // 摘要を取得する
        $cd["NAME"] = $cols2->item(0)->nodeValue;
        $cd["NAME"] = preg_replace('/-/', mb_convert_encoding('ー', 'UTF-8', 'sjis-win'), $cd["NAME"]);
        // 金額を取得する
        $cd["TRNAMT"] = (-1)*(double)parse_amount(trim($cols2->item(1)->nodeValue));
        
        array_push($cds, $cd);
    }
    return $cds;

}
?>
