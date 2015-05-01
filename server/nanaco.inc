<?php
/*
nanaco.inc: nanacoのHTMLよりOFXファイルを生成する
Copyright (C) 2014-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = str_replace("-", "", $ofxforms["XCID"]);
$pass = $ofxforms["LOGIN_PWD"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);

// PC版トップページを取得する
$as = parse_tag($body, "a");
$c = parse_tag_search($as, "innerHTML", "PC版トップページ");
if($c != -1) {
	$method = "GET";
	$uris = parse_uri($as[$c]["href"], $uris);
	$query = "";
	list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
}

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "マネー/ポイント残高履歴確認　クレジットチャージなど会員ログイン") != -1) {
	$method = "GET";
	$uris = parse_uri(($a["href"] == "javascript:WinOpenLogin()"? "https://www.nanaco-net.jp/pc/emServlet": $a["href"]), $uris);
	$query = "";
	list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
	break;
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "id", "login_password");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["XCID"] = "XCID=" . $user;
	$queries["LOGIN_PWD"] = "LOGIN_PWD=" . $pass;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = nanaco_update_cookie($head, $cookie);
	list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "残高・履歴確認") === false) {
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
	$account = array();
	
	$account["acctname"] = $settings["name"];
	
	// 支店番号を取得する
	$account["branchid"] = "0";
	
	// 口座番号を取得する
	$account["acctid"] = $user;
	
	// 残高を取得する
	$account["balance"] = 0;
	$divs = parse_tag($body, "div", true);
	$c = parse_tag_search($divs, "class", "moneyBox");
	if($c != -1) {
		$div2s = parse_tag($divs[$c]["innerHTML"], "div");
		$c = parse_tag_search($div2s, "class", "fRight");
		if($c != -1) $account["balance"] = parse_amount(strip_tags($div2s[$c]["innerHTML"]));
	}
	
	// 残高・履歴確認画面（1ページ目）を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "残高・履歴確認");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = nanaco_update_cookie($head, $cookie);
		list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
	}
	$account["details"] = array();
	
	// 無限ループする
	while(true) {
		// 明細を取得する
		$tables = parse_tag($body, "table");
		$c = parse_tag_search($tables, "id", "historyTbl");
		if($c != -1) $account["details"] = array_merge(nanaco_get_details($tables[$c]["innerHTML"]), $account["details"]);
		
		// 残高・履歴確認画面（2ページ目以降）を取得する
		$as = parse_tag($body, "a");
		$c = parse_tag_search($as, "innerHTML", " ＞＞次へ");
		if($c != -1) {
			$method = "GET";
			$uris = parse_uri($as[$c]["href"], $uris);
			$query = "";
			$cookie = nanaco_update_cookie($head, $cookie);
			list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
		} else {
			// 見つからない場合、ループを抜ける
			break;
		}
	}
	
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "ログアウト");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = nanaco_update_cookie($head, $cookie);
		list($head, $body) = nanaco_http11($method, $uris, $query, $cookie);
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	$bankmsgsrsv1 = "";
	$bankmsgsrsv1 .= "<BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// 口座情報を取得する
	$bankmsgsrsv1 .= "<STMTTRNRS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "<TRNUID>0</TRNUID>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "<STATUS><CODE>0</CODE><SEVERITY>INFO</SEVERITY></STATUS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "<STMTRS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "<CURDEF>" . ENV_STR_OFX_CURRENCY_JPY . "</CURDEF>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "<BANKACCTFROM>";
	$bankmsgsrsv1 .= "<BANKID>" . $settings["code"] . "</BANKID>";
	$bankmsgsrsv1 .= "<BRANCHID>" . $account["branchid"] . "</BRANCHID>";
	$bankmsgsrsv1 .= "<ACCTID>" . $account["acctid"] . "</ACCTID>";
	$bankmsgsrsv1 .= "<ACCTTYPE>" . ENV_STR_ACCTTYPE_CHECKING . "</ACCTTYPE>";
	$bankmsgsrsv1 .= "</BANKACCTFROM>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= nanaco_parse_details($account);
	$bankmsgsrsv1 .= "</STMTRS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "</STMTTRNRS>";
	$bankmsgsrsv1 .= "\r\n";
	
	$bankmsgsrsv1 .= "</BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	$resp["ofx"] = generate_ofx($resp["status"], $bankmsgsrsv1);
}
return $resp;

// HTTP/1.1
function nanaco_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.nanaco-net\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function nanaco_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID", "BIGipServerPool_443_pc"), parse_header($head, "set-cookie"), $cookie);
}

function nanaco_get_details($body) {
	$rets = array();
	$i = 0;
	$trs = parse_tag($body, "tr");
	foreach(array_reverse($trs) as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 10) {
			// 日時を取得する
			$dt = explode("<br>", $tds[0]["innerHTML"]);
			
			// 取引種別、および場所を取得する
			$name = trim(strip_tags($tds[1]["innerHTML"]));
			$memo = trim(strip_tags($tds[9]["innerHTML"]));
			
			// nanacoチャージ額・支払額を取得する
			$am = explode("<br>", $tds[2]["innerHTML"]);
			$amount = (string)((integer)parse_amount(trim(strip_tags($am[0]))) + (integer)parse_amount(trim(strip_tags(str_replace(array("(", ")"), array("", ""), $am[1])))));
			if($amount == "0") $amount = (string)(-1 * (integer)parse_amount(trim(strip_tags($tds[3]["innerHTML"]))));
			
			if($amount != "0") {
				$rets[$i]["date"] = parse_date(trim(strip_tags($dt[0])));
				$rets[$i]["summary"] = $name;
				$rets[$i]["memo"] = $memo;
				$rets[$i]["amount"] = $amount;
				
				$i++;
			}
		}
	}
	
	return $rets;
}

function nanaco_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	$dtstart = ENV_STR_DATE_PASTDAY;
	foreach($account["details"] as $line) {
		if($dtstart > $line["date"]) $dtstart = $line["date"];
		
		$cd = array();
		$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
		
		// 日付を取得する
		$cd["DTPOSTED"] = $line["date"];
		
		// 通番を生成する
		if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
		
		// トランザクション番号を生成する
		$cd["FITID"] = $cd["DTPOSTED"] . "0000000" . sprintf("%05d", $cd_num);
		
		// 摘要を取得する
		$cd["NAME"] = mb_convert_string($line["summary"]);
		
		// 金額を取得する
		$cd["TRNAMT"] = parse_amount($line["amount"]);
		$cd["MEMO"] = ($line["memo"] != ""? mb_convert_string($line["memo"]): ENV_STR_OFX_MEMO);
		
		array_push($cds, $cd);
		$cd_date = $cd["DTPOSTED"];
	}
	// BANKTRANLIST
	$ret .= "<BANKTRANLIST>";
	$ret .= "\r\n";
	$ret .= "<DTSTART>" . $dtstart . ENV_STR_OFX_TZ . "</DTSTART>";
	$ret .= "<DTEND>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTEND>";
	$ret .= "\r\n";
	
	foreach($cds as $cd) {
		$ret .= "<STMTTRN>";
		$ret .= "<TRNTYPE>" . $cd["TRNTYPE"] . "</TRNTYPE>";
		$ret .= "<DTPOSTED>" . $cd["DTPOSTED"] . ENV_STR_OFX_TZ . "</DTPOSTED>";
		$ret .= "<TRNAMT>" . $cd["TRNAMT"] . "</TRNAMT>";
		$ret .= "<FITID>" . $cd["FITID"] . "</FITID>";
		$ret .= "<NAME>" . $cd["NAME"] . "</NAME>";
		$ret .= "<MEMO>" . $cd["MEMO"] . "</MEMO>";
		$ret .= "</STMTTRN>";
		$ret .= "\r\n";
	}
	
	$ret .= "</BANKTRANLIST>";
	$ret .= "\r\n";
	$ret .= "<LEDGERBAL>";
	$ret .= "<BALAMT>" . $account["balance"] . "</BALAMT>";
	$ret .= "<DTASOF>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTASOF>";
	$ret .= "</LEDGERBAL>";
	$ret .= "\r\n";
	
	// 口座名称を出力する
	if($account["acctname"] != "") {
		$ret .= "<MKTGINFO>" . $account["acctname"] . "</MKTGINFO>";
		$ret .= "\r\n";
	}
	
	return $ret;
}

?>
