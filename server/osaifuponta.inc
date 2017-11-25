<?php
/*
osaifuponta.inc: おさいふPontaのCSVよりOFXファイルを生成する
Copyright (C) 2015-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["loginId"];
$pass = $ofxforms["password"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "マイページ") != -1) {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = osaifuponta_update_cookie($head, $cookie);
	list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
	break;
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "id", "memberLogInOutModel");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["loginId"] = "loginId=" . $user;
	$queries["password"] = "password=" . $pass;
	if(isset($queries["button-dummy"]) == true) unset($queries["button-dummy"]);
	if(isset($queries["btn_showRequestLoginIdNotice"]) == true) unset($queries["btn_showRequestLoginIdNotice"]);
	if(isset($queries["btn_showResetPassword"]) == true) unset($queries["btn_showResetPassword"]);
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = osaifuponta_update_cookie($head, $cookie);
	list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "メンテナンス中") !== false) { // 要検討
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回ログイン日時") === false) {
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
	$spans = parse_tag($body, "span");
	foreach($spans as $span) if(preg_match("/^[0-9]+$/", $span["innerHTML"]) > 0) {
		$account["acctid"] = strip_tags($span["innerHTML"]);
		break;
	}
	
	// 残高を取得する
	$tds = parse_tag($body, "td");
	foreach($tds as $td) if(strpos($td["innerHTML"], "チャージ残高") !== false) {
		$account["balance"] = parse_amount(trim(str_replace(array("チャージ残高", "　", "\\"), array("", "", ""), strip_tags($td["innerHTML"]))));
		break;
	}
	
	// 残高・取引明細照会画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "memberLogInOutModel");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["button-dummy"]) == true) unset($queries["button-dummy"]);
		if(isset($queries["menu_logout"]) == true) unset($queries["menu_logout"]);
		if(isset($queries["menu_top"]) == true) unset($queries["menu_top"]);
		if(isset($queries["menu_notice"]) == true) unset($queries["menu_notice"]);
		// if(isset($queries["menu_balanceDealDetailsSearch"]) == true) unset($queries["menu_balanceDealDetailsSearch"]); // 残高・取引明細照会
		if(isset($queries["menu_membershipInfo"]) == true) unset($queries["menu_membershipInfo"]);
		if(isset($queries["menu_cardInfo"]) == true) unset($queries["menu_cardInfo"]);
		if(isset($queries["menu_chargeAcceptance"]) == true) unset($queries["menu_chargeAcceptance"]);
		if(isset($queries["menu_registerCreditCard"]) == true) unset($queries["menu_registerCreditCard"]);
		if(isset($queries["menu_deleteCreditCard"]) == true) unset($queries["menu_deleteCreditCard"]);
		if(isset($queries["menu_pointComb"]) == true) unset($queries["menu_pointComb"]);
		if(isset($queries["btn_getNoticeList"]) == true) unset($queries["btn_getNoticeList"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = osaifuponta_update_cookie($head, $cookie);
		list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
	}
	
	// 明細を検索する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "retrieveBalanceInfoControllerModel");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["button-dummy"]) == true) unset($queries["button-dummy"]);
		if(isset($queries["menu_logout"]) == true) unset($queries["menu_logout"]);
		if(isset($queries["menu_top"]) == true) unset($queries["menu_top"]);
		if(isset($queries["menu_notice"]) == true) unset($queries["menu_notice"]);
		if(isset($queries["menu_balanceDealDetailsSearch"]) == true) unset($queries["menu_balanceDealDetailsSearch"]);
		if(isset($queries["menu_membershipInfo"]) == true) unset($queries["menu_membershipInfo"]);
		if(isset($queries["menu_cardInfo"]) == true) unset($queries["menu_cardInfo"]);
		if(isset($queries["menu_chargeAcceptance"]) == true) unset($queries["menu_chargeAcceptance"]);
		if(isset($queries["menu_registerCreditCard"]) == true) unset($queries["menu_registerCreditCard"]);
		if(isset($queries["menu_deleteCreditCard"]) == true) unset($queries["menu_deleteCreditCard"]);
		if(isset($queries["menu_pointComb"]) == true) unset($queries["menu_pointComb"]);
		// if(isset($queries["btn_searchRetrieveTransactionDetail"]) == true) unset($queries["btn_searchRetrieveTransactionDetail"]); // 検索
		if(isset($queries["btn_clear"]) == true) unset($queries["btn_clear"]);
		if(isset($queries["btn_getRetriveBalanceInfoCsvData"]) == true) unset($queries["btn_getRetriveBalanceInfoCsvData"]);
		if(isset($queries["btn_outputRetriveBalanceInfoPdf"]) == true) unset($queries["btn_outputRetriveBalanceInfoPdf"]);
		if(isset($queries["btn_getMemberTopInfo"]) == true) unset($queries["btn_getMemberTopInfo"]);
		
		$queries["retrieveTargetEraFrom"] = "retrieveTargetEraFrom=1"; // 西暦
		$queries["retrieveTargetDateFrom"] = "retrieveTargetDateFrom=" . ENV_STR_DATE_PASTDAY;
		$queries["retrieveTargetEraTo"] = "retrieveTargetEraTo=1"; // 西暦
		$queries["retrieveTargetDateTo"] = "retrieveTargetDateTo=" . ENV_STR_DATE_TODAY;
		$queries["rowNum"] = "rowNum=10"; // 表示件数
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = osaifuponta_update_cookie($head, $cookie);
		list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
	}
	
	$body_old = $body;
	
	// CSVファイルをダウンロードする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "retrieveBalanceInfoControllerModel");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["button-dummy"]) == true) unset($queries["button-dummy"]);
		if(isset($queries["menu_logout"]) == true) unset($queries["menu_logout"]);
		if(isset($queries["menu_top"]) == true) unset($queries["menu_top"]);
		if(isset($queries["menu_notice"]) == true) unset($queries["menu_notice"]);
		if(isset($queries["menu_balanceDealDetailsSearch"]) == true) unset($queries["menu_balanceDealDetailsSearch"]);
		if(isset($queries["menu_membershipInfo"]) == true) unset($queries["menu_membershipInfo"]);
		if(isset($queries["menu_cardInfo"]) == true) unset($queries["menu_cardInfo"]);
		if(isset($queries["menu_chargeAcceptance"]) == true) unset($queries["menu_chargeAcceptance"]);
		if(isset($queries["menu_registerCreditCard"]) == true) unset($queries["menu_registerCreditCard"]);
		if(isset($queries["menu_deleteCreditCard"]) == true) unset($queries["menu_deleteCreditCard"]);
		if(isset($queries["menu_pointComb"]) == true) unset($queries["menu_pointComb"]);
		if(isset($queries["btn_searchRetrieveTransactionDetail"]) == true) unset($queries["btn_searchRetrieveTransactionDetail"]);
		if(isset($queries["btn_clear"]) == true) unset($queries["btn_clear"]);
		// if(isset($queries["btn_getRetriveBalanceInfoCsvData"]) == true) unset($queries["btn_getRetriveBalanceInfoCsvData"]); // CSVダウンロード
		if(isset($queries["btn_outputRetriveBalanceInfoPdf"]) == true) unset($queries["btn_outputRetriveBalanceInfoPdf"]);
		if(isset($queries["btn_getMemberTopInfo"]) == true) unset($queries["btn_getMemberTopInfo"]);
		
		$queries["retrieveTargetEraFrom"] = "retrieveTargetEraFrom=1"; // 西暦
		$queries["retrieveTargetDateFrom"] = "retrieveTargetDateFrom=" . ENV_STR_DATE_PASTDAY;
		$queries["retrieveTargetEraTo"] = "retrieveTargetEraTo=1"; // 西暦
		$queries["retrieveTargetDateTo"] = "retrieveTargetDateTo=" . ENV_STR_DATE_TODAY;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = osaifuponta_update_cookie($head, $cookie);
		list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = osaifuponta_update_cookie($head, $cookie);
				list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
			}
		}
	}
	
	// 明細を取得する
	if(strpos($body, "システムエラー画面") === false) {
		$account["details"] = osaifuponta_get_details($body);
	} else {
		$account["details"] = array();
	}
	
	$body = $body_old;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "retrieveBalanceInfoControllerModel");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["button-dummy"]) == true) unset($queries["button-dummy"]);
		// if(isset($queries["menu_logout"]) == true) unset($queries["menu_logout"]); // ログアウト
		if(isset($queries["menu_top"]) == true) unset($queries["menu_top"]);
		if(isset($queries["menu_notice"]) == true) unset($queries["menu_notice"]);
		if(isset($queries["menu_balanceDealDetailsSearch"]) == true) unset($queries["menu_balanceDealDetailsSearch"]);
		if(isset($queries["menu_membershipInfo"]) == true) unset($queries["menu_membershipInfo"]);
		if(isset($queries["menu_cardInfo"]) == true) unset($queries["menu_cardInfo"]);
		if(isset($queries["menu_chargeAcceptance"]) == true) unset($queries["menu_chargeAcceptance"]);
		if(isset($queries["menu_registerCreditCard"]) == true) unset($queries["menu_registerCreditCard"]);
		if(isset($queries["menu_deleteCreditCard"]) == true) unset($queries["menu_deleteCreditCard"]);
		if(isset($queries["menu_pointComb"]) == true) unset($queries["menu_pointComb"]);
		if(isset($queries["retrieveTargetEraFrom"]) == true) unset($queries["retrieveTargetEraFrom"]);
		if(isset($queries["retrieveTargetDateFrom"]) == true) unset($queries["retrieveTargetDateFrom"]);
		if(isset($queries["retrieveTargetEraTo"]) == true) unset($queries["retrieveTargetEraTo"]);
		if(isset($queries["retrieveTargetDateTo"]) == true) unset($queries["retrieveTargetDateTo"]);
		if(isset($queries["btn_searchRetrieveTransactionDetail"]) == true) unset($queries["btn_searchRetrieveTransactionDetail"]);
		if(isset($queries["btn_clear"]) == true) unset($queries["btn_clear"]);
		if(isset($queries["btn_getRetriveBalanceInfoCsvData"]) == true) unset($queries["btn_getRetriveBalanceInfoCsvData"]); // CSVダウンロード
		if(isset($queries["btn_outputRetriveBalanceInfoPdf"]) == true) unset($queries["btn_outputRetriveBalanceInfoPdf"]);
		if(isset($queries["btn_getMemberTopInfo"]) == true) unset($queries["btn_getMemberTopInfo"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = osaifuponta_update_cookie($head, $cookie);
		list($head, $body) = osaifuponta_http11($method, $uris, $query, $cookie);
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
	$bankmsgsrsv1 .= osaifuponta_parse_details($account);
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
function osaifuponta_http11($method, $uris, $query = "", $cookie = "", $referer = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.osaifuponta\.lawson\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie, true, ENV_PRODUCT_UA, "", $referer);
	return explode("\r\n\r\n", $ret, 2);
}

function osaifuponta_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID", "BIGipServerppbstlb-19-vip01_pool"), parse_header($head, "set-cookie"), $cookie);
}

function osaifuponta_get_details($csv) {
	$rets = array();
	$i = 0;
	$lines = array_reverse(parse_csv(mb_convert_string($csv)));
	foreach($lines as $line) {
		if($line[0] != "" && $line[0] != "行") {
			$rets[$i]["date"] = parse_date($line[2]);
			$rets[$i]["summary"] = $line[3];
			$rets[$i]["amount"] = parse_amount($line[4]);
			$rets[$i]["memo"] = $line[1];
			if($line[1] == "お買い物" || $line[1] == "お買い物（確定前）") $rets[$i]["amount"] = "-" . $rets[$i]["amount"];
			if($line[3] == "") {
				$rets[$i]["summary"] = $line[1];
				$rets[$i]["memo"] = ENV_STR_OFX_MEMO;
			}
			$i++;
		}
	}
	return $rets;
}

function osaifuponta_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	foreach($account["details"] as $line) {
		$cd = array();
		$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
		
		// 日付を取得する
		$cd["DTPOSTED"] = $line["date"];
		
		// 通番を生成する
		if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
		
		// トランザクション番号を生成する
		$cd["FITID"] = $cd["DTPOSTED"] . "0000000" . sprintf("%05d", $cd_num);
		
		// 摘要を取得する
		$cd["NAME"] = $line["summary"];
		
		// 金額を取得する
		$cd["TRNAMT"] = parse_amount($line["amount"]);
		$cd["MEMO"] = ($line["memo"] != ""? $line["memo"]: ENV_STR_OFX_MEMO);
		
		array_push($cds, $cd);
		$cd_date = $cd["DTPOSTED"];
	}
	// BANKTRANLIST
	$ret .= "<BANKTRANLIST>";
	$ret .= "\r\n";
	$ret .= "<DTSTART>" . ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ . "</DTSTART>";
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
