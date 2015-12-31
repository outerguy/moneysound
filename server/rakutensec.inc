<?php
/*
rakutensec.inc: 楽天証券のHTMLよりOFXファイルを生成する
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
list($head, $body) = rakutensec_http11('GET', $uris, $query, $cookie);

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "loginform");
$inputs = parse_tag($forms[$c]["innerHTML"], "input");
$queries = array();
foreach($inputs as $input) {
    if($input["name"] != "") {
        $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
    }
}
$queries["loginid"] .= $ofxforms["loginid"];
$queries["passwd"] .= $ofxforms["passwd"];
$queries["homeid"] = "homeid=HOME";
$uris = parse_uri('https://member.rakuten-sec.co.jp/bv/app/MhLogin.do', $uris);
$query = implode("&", $queries);
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body) = rakutensec_http11('POST', $uris, $query, $cookie);

if (strpos($body, "未実装") !== false) {
    // システムメンテナンスの場合
    $resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
    return $resp;
} else if (!preg_match("/\"(\/app\/home\.do.*)\"/", $body, $matches)) {
    // ログイン失敗の場合
    $resp["status"] = ENV_NUM_STATUS_FAILURE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
    return $resp;
}

// リダイレクトし、ホーム画面のHTMLを取得する
$uris = parse_uri($matches[1], $uris);
preg_match('/(BV_SessionID=.*)\?/', $matches[1], $matches);
$sessionid = $matches[1];
$query = "";
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_home) = rakutensec_http11('GET', $uris, $query, $cookie);

// 資産情報のHTMLを取得する
$uris['path'] = '/app/async_change_home_balance_lst.do;' . $sessionid;
$query = 'updateKbn=lastDay&openCode=1';
list($head, $body_balance) = rakutensec_http11('POST', $uris, $query, $cookie);

// 入出金履歴のCSVを取得する
// 一度ページ遷移をしないと、CSVをダウンロードできない模様
$uris['path'] = '/app/ass_money_trans_lst.do;' . $sessionid;
$query = 'eventType=init';
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_trans) = rakutensec_http11('GET', $uris, $query, $cookie);

$uris['path'] = '/app/ass_money_trans_lst_csv_output.do;' . $sessionid;
$query = 'eventType=csv';
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_trans) = rakutensec_http11('GET', $uris, $query, $cookie);

// 取引履歴（国内株式）のCSVを取得する
$uris['path'] = '/app/assTradJpLstCsvServlet;' . $sessionid;
$queries = array();
$queries['searchFlg'] = 'searchFlg=0';
$queries['termCd'] = 'termCd=ALL';
$queries['trustKbn'] = 'trustKbn=A';
$queries['accCd'] = 'accCd=A';
foreach(array('yyyyFrom', 'mmFrom', 'ddFrom',
              'yyyyTo', 'mmTo', 'ddTo',
              'dscrCdNm',
              'hiddenSearchFlag',
              'hiddenYearFrom', 'hiddenMonthFrom', 'hiddenDayFrom',
              'hiddenYearTo', 'hiddenMonthTo', 'hiddenDayTo',
              'hiddenSpotTrustKbn', 'hiddenAccountCd', 'hiddenDscr',
              'offsetRow') as $key) {
    $queries[$key] = $key . '=';
}
$query = implode("&", $queries);
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_trade_jp) = rakutensec_http11('GET', $uris, $query, $cookie);

// 取引履歴（投資信託）のCSVを取得する
$uris['path'] = '/app/assTradFuLstCsvServlet;' . $sessionid;
unset($queries['trustKbn']);
unset($queries['hiddenSpotTrustKbn']);
$queries['hiddenTermCd'] = 'hiddenTermCd=';
$query = implode("&", $queries);
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_trade_mf) = rakutensec_http11('GET', $uris, $query, $cookie);

// 保有商品一覧（国内株式）のCSVを取得する
$uris['path'] = '/app/ass_jp_possess_lst_csv_output.do;' . $sessionid;
$query = 'eventType=csv';
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_pos_jp) = rakutensec_http11('GET', $uris, $query, $cookie);

// 保有商品一覧（投資信託）のCSVを取得する
$uris['path'] = '/app/ass_fu_possess_lst_csv_output.do;' . $sessionid;
$query = 'eventType=csv';
$cookie = rakutensec_update_cookie($head, $cookie);
list($head, $body_pos_mf) = rakutensec_http11('GET', $uris, $query, $cookie);

// ログアウトする
$uris['path'] = '/app/logout.do;' . $sessionid;
$query = '';
list($head, $body) = rakutensec_http11('GET', $uris, $query, $cookie);

// リダイレクトする
$uris['path'] = '/app/logout;' . $sessionid;
list($head, $body) = rakutensec_http11('GET', $uris, $query, $cookie);

/********** ページ遷移ここまで **********/

// ホーム画面のHTMLを処理し、部店-お客様コード（口座番号）を取得する
if (preg_match('/[0-9]{3}-[0-9]{6}/', $body_home, $matches)) $acctid = $matches[0];

// DOMツリーを生成
$ofxdom = new ofxDOM("INVSTMT", $settings["name"]);
$ofxdom->setAcctfrom(array(
    "BROKERID" => $settings["code"],
    "ACCTID" => $acctid
));

// 資産情報のHTMLを処理する
$divs = parse_tag($body_balance, 'div');
$c = parse_tag_search($divs, 'id', 'balance_data_actual_data');
$tds = parse_tag($divs[$c]['innerHTML'], 'td');
preg_match('/[0-9,]+/', $tds[21]['innerHTML'], $matches);
$availcash = parse_amount($matches[0]);

// 残高を設定
$ofxdom->setBalance(array(
    'AVAILCASH' => $availcash,
    'MARGINBALANCE' => $availcash,
    'SHORTBALANCE' => 0
));

// 入出力履歴のCSVを処理する
$body_trans = mb_convert_encoding($body_trans, "UTF-8", "sjis-win");
$rows = parse_csv($body_trans);
array_splice($rows, 0, 4); // ヘッダー行等最初の４行（＋空行１行）をスキップする
foreach ($rows as $row) {
    $cd = array();
    $date = date_parse($row[0]);
    $cd['DTPOSTED'] = sprintf("%d%02d%02d", $date['year'], $date['month'], $date['day']);
    $cd['DTPOSTED'] .= ENV_STR_OFX_TZ;
    if (preg_match('/[0-9,]+/', $row[1]) > 0) { // 入金
        $cd['TRNAMT'] = $row[1];
    } else { // 出金
        $cd['TRNAMT'] = '-' . $row[2];
    }
    $cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
    $cd['NAME'] = $row[3] . ' ' . $row[4];
    // トランザクション番号（請求月とデータ種別）を生成する
    $cd["FITID"] = '000';

    // 入出力履歴をDOMに書き込む
    if (strtotime(ENV_STR_DATE_PASTDAY) <= strtotime($row[0]) && strtotime($row[0]) <= strtotime(ENV_STR_DATE_TODAY)) {
        $ofxdom->addTran($cd);
    } else {
        break;
    }
}

// DTSTARTとDTENDを設定する
$ofxdom->setDateRange(ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);

// 取引履歴（国内株式）のCSVを処理する
$body_trade_jp = mb_convert_encoding($body_trade_jp, "UTF-8", "sjis-win");
$rows = parse_csv($body_trade_jp);
array_shift($rows); // ヘッダー行をスキップする
foreach ($rows as $row) {
    $ct = array();
    $ct['CATEGORY'] = ENV_STR_OFX_STOCK;
    $date = date_parse($row[0]);
    $ct['DTTRADE'] = sprintf("%d%02d%02d000000", $date['year'], $date['month'], $date['day'])
                     . ENV_STR_OFX_TZ;
    $ct['UNIQUEID'] = $row[2];
    $ct['SECNAME'] = $row[3];
    $ct['UNIQUEIDTYPE'] = ENV_STR_OFX_CODE_STOCK;
    $ct['UNITS'] = (int)parse_amount($row[10]);
    $ct['UNITPRICE'] = parse_amount($row[11]);
    $ct['FEES'] = (int)parse_amount($row[12]);
    $ct['TAXES'] = (int)parse_amount($row[13]);
    $ct['COMMISSION'] = (int)parse_amount($row[14]);
    $ct['TOTAL'] = (int)parse_amount($row[16]);
    $ct['SUBACCTSEC'] = ENV_STR_OFX_CASH;
    $ct['SUBACCTFUND'] = ENV_STR_OFX_CASH;
    $ct['FITID'] = '000';
    if (preg_match('/買付/', $row[7]) > 0) {
        $ct['BUYTYPE'] = ENV_STR_OFX_BUY;
        $ct['TOTAL'] *= -1;
    } else {
        $ct['BUYTYPE'] = ENV_STR_OFX_SELL;
        $ct['UNITS'] *= -1;
    }

    if (strtotime(ENV_STR_DATE_PASTDAY) <= strtotime($row[0]) && strtotime($row[0]) <= strtotime(ENV_STR_DATE_TODAY)) {
        $ofxdom->addTrade($ct);
    }
}

// 取引履歴（投資信託）のCSVを処理する
$body_trade_mf = mb_convert_encoding($body_trade_mf, "UTF-8", "sjis-win");
$rows = parse_csv($body_trade_mf);
array_shift($rows); // ヘッダー行をスキップする
foreach ($rows as $row) {
    $ct = array();
    $ct['CATEGORY'] = ENV_STR_OFX_FUND;
    $date = date_parse($row[0]);
    $ct['DTTRADE'] = sprintf("%d%02d%02d", $date['year'], $date['month'], $date['day'])
                     . ENV_STR_OFX_TZ;
    $ct['UNIQUEID'] = md5($row[2]);
    $ct['SECNAME'] = $row[2];
    $ct['UNIQUEIDTYPE'] = ENV_STR_OFX_CODE_FUND;
    $ct['UNITS'] = (int)parse_amount($row[7]);
    $ct['UNITPRICE'] = parse_amount($row[8]);
    //$ct['FEES'] = (int)parse_amount($row[9]);
    // total = units * unitprice + (commission + fee + taxes)とするため、feeを0にする
    $ct['FEES'] = 0;
    $ct['TAXES'] = 0;
    $ct['COMMISSION'] = 0;
    $ct['TOTAL'] = (int)parse_amount($row[10]);
    $ct['SUBACCTSEC'] = ENV_STR_OFX_CASH;
    $ct['FITID'] = '000';
    switch ($row[5]) {
        case '買付':
            $ct['BUYTYPE'] = ENV_STR_OFX_BUY;
            $ct['SUBACCTFUND'] = ENV_STR_OFX_CASH;
            $ct['TOTAL'] *= -1;
            break;
        case '解約':
            $ct['BUYTYPE'] = ENV_STR_OFX_SELL;
            $ct['SUBACCTFUND'] = ENV_STR_OFX_CASH;
            $ct['UNITS'] *= -1;
            break;
        case '再投資':
        default:
            $ct['BUYTYPE'] = ENV_STR_OFX_REINVEST;
            $ct['INCOMETYPE'] = ENV_STR_OFX_TRNTYPE_INT;
            $ct['TOTAL'] *= -1;
            break;
    }

    if (strtotime(ENV_STR_DATE_PASTDAY) <= strtotime($row[0]) && strtotime($row[0]) <= strtotime(ENV_STR_DATE_TODAY)) {
        $ofxdom->addTrade($ct);
    }
}

// 保有商品一覧（国内株式）のCSVを処理する
$body_pos_jp = mb_convert_encoding($body_pos_jp, "UTF-8", "sjis-win");
$rows = parse_csv($body_pos_jp);
array_shift($rows); // ヘッダー行をスキップする
foreach ($rows as $row) {
    $cl = array();
    $cl['CATEGORY'] = ENV_STR_OFX_STOCK;
    $cl['HELDINACCT'] = ENV_STR_OFX_CASH;
    $cl['POSTYPE'] = 'LONG';
    $cl['DTPRICEASOF'] = ENV_STR_DATE_TODAY;
    $cl['UNIQUEIDTYPE'] = ENV_STR_OFX_CODE_STOCK;
    $cl['UNIQUEID'] = $row[1];
    $cl['SECNAME'] = $row[2];
    $cl['UNITS'] = (int)parse_amount($row[3]);
    $cl['UNITPRICE'] = parse_amount($row[6]);
    $cl['MKTVAL'] = (int)parse_amount($row[7]);
    $cl['MEMO'] = (int)parse_amount($row[8]);

    $ofxdom->addPos($cl);
    $ofxdom->addSec($cl);
}

// 保有商品一覧（投資信託）のCSVを処理する
$body_pos_mf = mb_convert_encoding($body_pos_mf, "UTF-8", "sjis-win");
$rows = parse_csv($body_pos_mf);
array_shift($rows); // ヘッダー行をスキップする
foreach ($rows as $row) {
    $cl = array();
    $cl['CATEGORY'] = ENV_STR_OFX_FUND;
    $cl['HELDINACCT'] = ENV_STR_OFX_CASH;
    $cl['POSTYPE'] = 'LONG';
    $cl['DTPRICEASOF'] = ENV_STR_DATE_TODAY;
    $cl['UNIQUEID'] = md5($row[2]);
    $cl['UNIQUEIDTYPE'] = ENV_STR_OFX_CODE_FUND;
    $cl['SECNAME'] = $row[2];
    $cl['UNITS'] = (int)parse_amount($row[4]);
    $cl['UNITPRICE'] = 1;
    $cl['MKTVAL'] = (int)parse_amount($row[14]);
    $cl['MEMO'] = $row[10];

    $ofxdom->addPos($cl);
    $ofxdom->addSec($cl);
}

// FITIDを仕上げる
$ofxdom->setFitid();
// XML DOMツリーを文字列に変換
$xml = $ofxdom->getXML();
$resp["status"] = ENV_NUM_STATUS_SUCCESS;
$resp["ofx"] = generate_ofx($mode, $xml);

return $resp;

// HTTP/1.1
function rakutensec_http11($method, $uris, $query = "", $cookie = "") {
    $ret = "INVALID HOST";
    if(preg_match("/\.rakuten-sec\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
    return explode("\r\n\r\n", $ret, 2);
}
function rakutensec_update_cookie($head, $cookie) {
    return update_cookie(array(
            "Rg_sec",
            "checkTk",
        ), parse_header($head, "set-cookie"), $cookie);
}
?>
