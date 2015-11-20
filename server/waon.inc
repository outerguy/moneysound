<?php
/*
waon.inc: WAONネットステーションのHTMLよりOFXファイルを生成する
Copyright (C) 2012-2015 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Licensed under the GNU AGPLv3.
*/

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ログイン画面を取得する
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = waon_http11('GET', $uris, $query, $cookie);

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "loginForm");
$inputs = parse_tag($forms[$c]["innerHTML"], "input");
$queries = array();
foreach($inputs as $input) {
    if($input["name"] != "") {
        $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
    }
}
$queries["cardNo1"] .= substr($ofxforms["cardNo"], 0, 4);
$queries["cardNo2"] .= substr($ofxforms["cardNo"], 4, 4);
$queries["cardNo3"] .= substr($ofxforms["cardNo"], 8, 4);
$queries["cardNo4"] .= substr($ofxforms["cardNo"], 12, 4);
$queries["secNo"] .= $ofxforms["secNo"];
$uris = parse_uri('/wmUseHistoryInq/login.do', $uris);
$query = implode("&", $queries);
$cookie = waon_update_cookie($head, $cookie);
list($head, $body) = waon_http11('POST', $uris, $query, $cookie);

if (strpos($body, "未実装") !== false) {
    // システムメンテナンス画面の場合
    $resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
} else if (strpos($body, "が正しく入力されていません。") !== false) {
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
    // 今月の明細を処理する
    $cds = waon_parsedom($body);

    // 前月の明細を取得する
    $as = parse_tag($body, "a");
    foreach($as as $a) {
        if (preg_match("/.*beforeMonth=1.*/", $a["href"])) { $url = $a["href"]; }
    }
    $urls = parse_url($url);
    $queries = array();
    foreach(explode("&", htmlspecialchars_decode($urls["query"])) as $pair) {
        list($key, $value) = explode("=", $pair);
        $queries[$key] = $key . "=" . $value;
    }
    $uris = parse_uri($urls["path"], $uris);
    $query = implode("&", $queries);
    $cookie = waon_update_cookie($head, $cookie);
    list($head, $body) = waon_http11('GET', $uris, $query, $cookie);
    // 前月の明細を処理する
    $cds = array_merge($cds, waon_parsedom($body));

    // ログアウトする
    $forms = parse_tag($body, "form");
    $c = parse_tag_search($forms, "name", "logoutForm");
    $inputs = parse_tag($forms[$c]["innerHTML"], "input");
    $queries = array();
    foreach($inputs as $input) {
        if($input["name"] != "") {
            $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
        }
    }
    $uris = parse_uri('/wmUseHistoryInq/logout.do', $uris);
    $query = implode("&", $queries);
    $cookie = waon_update_cookie($head, $cookie);
    list($head, $body) = waon_http11('POST', $uris, $query, $cookie);

    // DOMツリーを生成
    $ofxdom = new ofxDOM("BANK");
    $ofxdom->setAcctfrom(array(
        "BANKID" => $settings["code"],
        "BRANCHID" => "0",
        "ACCTID" => $ofxforms["cardNo"],
        "ACCTTYPE" => "CHECKING"
    ));
    // 日付順に昇順ソート
    usort($cds, function($a, $b) {
        return $a['DTPOSTED'] > $b['DTPOSTED'];
    });
    $ofxdom->setTrans($cds);
    // TRANLIST
    $ofxdom->setTranList(ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);

    // 残高を処理。ただし、取得できないため、便宜上0円とする。
    $ofxdom->setLedgerBalance(0, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);

    //MKGTINFOを設定
    $ofxdom->setMktginfo($settings["name"]);

    // XML DOMツリーを文字列に変換
    $xml = $ofxdom->getXML();
    $resp["status"] = ENV_NUM_STATUS_SUCCESS;
    $resp["ofx"] = generate_ofx($mode, $xml);
}
return $resp;

// HTTP/1.1
function waon_http11($method, $uris, $query = "", $cookie = "") {
    $ret = "INVALID HOST";
    if(preg_match("/\.waon\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
    return explode("\r\n\r\n", $ret, 2);
}
function waon_update_cookie($head, $cookie) {
    return update_cookie(array(
            "JSESSIONID",
            "citrix_ns_id_.waon.com_%2F_wat",
            "Money-VS32-Cookie",
            "citrix_ns_id",
            "Waon-SSLVS04-Cookie",
        ), parse_header($head, "set-cookie"), $cookie);
}

function waon_parsedom($str) {
    $str = str_replace("Shift_JIS", "UTF-8", $str);
    $doc = new DOMDocument();
    @$doc->loadHTML($str);
    $xpath = new DOMXPath($doc);
    $rows = $xpath->query("//table[@class='y']/tr");

    $cds = array();
    $nrow = $rows->length;

    for ($i=1; $i<$nrow; $i++) {
        $cd = array();
        $cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;

        $row = $rows->item($i);
        $cols = $row->getElementsByTagName('td');
        // 日付を取得する
        if (!preg_match("/([0-9]{4})\/([0-9]{1,2})\/([0-9]{1,2})/", trim($cols->item(0)->nodeValue), $matches)) {
            // 当月に明細が存在しなければスキップ
            break;
        }
        $cd["DTPOSTED"] = sprintf("%d%02d%02d", $matches[1], $matches[2], $matches[3]);

        // トランザクション番号（請求月とデータ種別）を生成する
        $cd["FITID"] = '000';

        $cd['NAME'] = trim($cols->item(1)->nodeValue);
        $cd['TRNAMT'] = (int)parse_amount(trim($cols->item(2)->nodeValue));
        if (preg_match("/支払/", $cols->item(3)->nodeValue)) {
            $cd['TRNAMT'] *= -1;
        }
        array_push($cds, $cd);
    }
    return $cds;
}
?>
