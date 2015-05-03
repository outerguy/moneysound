<?php
/*
mobilesuica.inc: モバイルSuicaのHTMLよりOFXファイルを生成する
Copyright (C) 2014-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["MailAddress"];
$pass = $ofxforms["Password"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["WebCaptcha1__editor"]), "Shift_JIS", "UTF-8"));
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];

if($sesscookie != "") list($cookie, $dummy) = explode("\t", sess_decode($sesscookie));
if($accesskey != "") {
	list($ssid, $method, $uri, $query, $user, $dummy) = explode("\t", sess_decode($accesskey), 6);
	$sid = (integer)$ssid;
}

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

if($sid == 0) {
	// ホーム画面（ログイン画面）を取得する
	$method = "GET";
	$uris = parse_uri($settings["home"]);
	$query = "";
	$cookie = "";
	list($head, $body) = mobilesuica_http11($method, $uris, $query, $cookie);
	
	// 画像認証の画像を取得する
	$imgs = parse_tag($body, "img");
	$c = parse_tag_search($imgs, "class", "igc_TrendyCaptchaImage");
	if($c != -1) {
		$imguris = parse_uri($imgs[$c]["src"], $uris);
		$cookie = mobilesuica_update_cookie($head, $cookie);
		list($imghead, $imgbody) = mobilesuica_http11($method, $imguris, $query, $cookie, $uris["scheme"] . "://" . $uris["host"] . $uris["path"] . $query, false); // 文字コードを変換しない
		$imgsrc = "data:image/gif;base64," . base64_encode($imgbody);
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "form1");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["MailAddress"] = "MailAddress=" . $user;
		$queries["Password"] = "Password=" . $pass;
		$queries["WebCaptcha1_clientState"] = "WebCaptcha1_clientState=%5B%5B%5B%5Bnull%2Cnull%5D%5D%2C%5B%5D%2C%5B%5D%5D%2C%5B%7B%7D%2C%5B%5D%5D%2Cnull%5D";
		$queries["WebCaptcha1__editor_clientState"] = "WebCaptcha1__editor_clientState=%7C0%7C01%7C%7C%5B%5B%5B%5B%5D%5D%2C%5B%5D%2C%5B%5D%5D%2C%5B%7B%7D%2C%5B%5D%5D%2C%22CAPTCHA%22%5D";
		$queries["WebCaptcha1__editor"] = "WebCaptcha1__editor=";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mobilesuica_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("WebCaptcha1__editor=", "WebCaptcha1__editor=" . $auth, $query);
	$query = str_replace("CAPTCHA", sprintf("%02d", strlen($auth)) . $auth, $query);
	list($head, $body) = mobilesuica_http11($method, $uris, $query, $cookie);
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "WebCaptcha1__editor";
	$resp["addition"] = $imgsrc;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $user, ENV_STR_SESSION_PADDING)));
	
	$mfachallengetrnrs = "";
	$mfachallengetrnrs .= "<MFACHALLENGETRNRS>";
	$mfachallengetrnrs .= "<MFACHALLENGERS>";
	$mfachallengetrnrs .= "<MFACHALLENGE>";
	$mfachallengetrnrs .= "<MFAPHRASEID>" . $resp["aid"] . "</MFAPHRASEID>";
	$mfachallengetrnrs .= "<MFAPHRASELABEL>" . $resp["addition"] . "</MFAPHRASELABEL>";
	$mfachallengetrnrs .= "</MFACHALLENGE>";
	$mfachallengetrnrs .= "</MFACHALLENGERS>";
	$mfachallengetrnrs .= "</MFACHALLENGETRNRS>";
	
	$resp["ofx"] = generate_ofx($resp["status"], $mfachallengetrnrs, $resp["sesscookie"], $resp["accesskey"]);
} else if(strpos($body, "SF(電子マネー)利用履歴") === false) {
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
	
	// SF（電子マネー）利用履歴画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "SF(電子マネー)利用履歴") != -1) {
		$method = "POST";
		if(preg_match("/javascript:[^\']+?\'([^\']+)\'.*?/i", $a["href"], $matches) > 0) $uris = parse_uri($matches[1], $uris);
		$query = "";
		$cookie = mobilesuica_update_cookie($head, $cookie);
		list($head, $body) = mobilesuica_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$account["details"] = array();
	$tables = parse_tag($body, "table", true);
	foreach($tables as $table) if($table["cellpadding"] == "8") {
		$account["details"] = mobilesuica_get_details($table["innerHTML"]);
		$detail = array_shift($account["details"]);
		$account["balance"] = $detail["amount"];
	}
	
	// SuicaID番号を取得する
	$forms = parse_tag($body, "form", true);
	$c = parse_tag_search($forms, "name", "form1");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$selects = parse_tag($forms[$c]["innerHTML"], "select");
		foreach($selects as $select) {
			$options = parse_tag($select["innerHTML"], "option");
			foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
		}
		$queries["baseVarCopy"] = "baseVarCopy=" . substr($queries["baseVariable"], strpos($queries["baseVariable"], "=") + 1);
		if($queries["SEARCH"] != "") unset($queries["SEARCH"]);
		if($queries["RETURNMENU"] != "") unset($queries["RETURNMENU"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mobilesuica_update_cookie($head, $cookie);
		list($head, $body) = mobilesuica_http11($method, $uris, $query, $cookie);
		
		$contentdispositions = parse_header($head, "content-disposition");
		if(count($contentdispositions) > 0) {
			$account["acctid"] = substr($contentdispositions[0], strpos($contentdispositions[0], "=") + 1, 17);
		}
	
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$method = "POST";
	$uris = parse_uri("/ka/lg/LogoutComplete.aspx", $uris);
	$query = "";
	$cookie = mobilesuica_update_cookie($head, $cookie);
	list($head, $body) = mobilesuica_http11($method, $uris, $query, $cookie);
	
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
	$bankmsgsrsv1 .= mobilesuica_parse_details($account);
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
function mobilesuica_http11($method, $uris, $query = "", $cookie = "", $referer = "", $autoconv = true) {
	$ret = "INVALID HOST";
	if(preg_match("/\.mobilesuica\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie, $autoconv, ENV_PRODUCT_UA . " MSIE 7.0", "", $referer); // ログイン時にUser-Agentをチェックされる
	return explode("\r\n\r\n", $ret, 2);
}

function mobilesuica_update_cookie($head, $cookie) {
	return update_cookie(array("ASP.NET_SessionId", "sc_auth"), parse_header($head, "set-cookie"), $cookie);
}

function mobilesuica_get_details($body) {
	$rets = array();
	$trs = parse_tag($body, "tr");
	array_shift($trs);
	$i = count($trs) - 1;
	$prev = "";
	$last = "";
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 7) {
			// 日時を取得する
			$dt = trim(str_replace("/", "", strip_tags($tds[0]["innerHTML"])));
			if($dt > substr(ENV_STR_DATE_TODAY, 4, 4)) {
				$dt = (string)((integer)substr(ENV_STR_DATE_TODAY, 0, 4) - 1) . $dt;
			} else {
				$dt = substr(ENV_STR_DATE_TODAY, 0, 4) . $dt;
			}
			
			// 取引種別、および場所を取得する
			$class1 = mobilesuica_parse_string($tds[1]["innerHTML"]);
			$place1 = mobilesuica_parse_string($tds[2]["innerHTML"]);
			$class2 = mobilesuica_parse_string($tds[3]["innerHTML"]);
			$place2 = mobilesuica_parse_string($tds[4]["innerHTML"]);
			
			$name = $class1 . $class2;
			$memo = $place1;
			if($place2 != "") $memo .= "-" . $place2;
			if($memo == "") $memo = ENV_STR_OFX_MEMO;
			
			// 収支を計算する
			$amount = mobilesuica_parse_string($tds[6]["innerHTML"]);
			
			// 残高を取得する
			if($last == "") $last = parse_amount(str_replace("&yen;", "", trim(strip_tags($tds[5]["innerHTML"]))));
			
			$rets[$i]["date"] = $dt;
			$rets[$i]["summary"] = $name;
			$rets[$i]["amount"] = $amount;
			$rets[$i]["memo"] = $memo;
			
			$i--;
		}
	}
	$rets[0]["amount"] = $last;
	
	// 戻り値の配列の先頭が残高となる（明細としては無効）
	return array_reverse($rets);
}

function mobilesuica_parse_string($str) {
	return trim(mb_convert_kana(strip_tags($str), "sKV", "UTF-8"));
}

function mobilesuica_parse_details($account) {
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
