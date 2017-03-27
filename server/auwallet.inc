<?php
/*
auwallet.inc: au WALLETのCSV/HTMLよりOFXファイルを生成する
Copyright (C) 2014-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["loginAliasId"];
$pass = $ofxforms["loginAuonePwd"];
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];
$token = urldecode($ofxforms["X-Token"]);

$resp = array();
$sid = 0;
$method = "";
$query = "";
$cookie = "";
$head = "";
$body = "";

if($sesscookie != "") list($cookie, $dummy) = explode("\t", sess_decode($sesscookie));
if($accesskey != "") {
	list($ssid, $method, $uri, $query, $user, $pass, $dummy) = explode("\t", sess_decode($accesskey), 7);
	$sid = (integer)$ssid;
}

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

if($sid == 0) {
	// ホーム画面を取得する
	$method = "GET";
	$uris = parse_uri($settings["home"]);
	$query = "";
	$cookie = (ENV_BOOL_ADD_RISKBASE == true && $token != ""? $token: "");
	list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	
	// ログイン画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "login");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = auwallet_update_cookie($head, $cookie);
				list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
			}
		}
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "main");
	if($c != -1) {
		// $inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$inputs = parse_tag($body, "input"); // 2015年11月17日に発生したサイトの不具合に暫定対処する
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["loginAliasId"] = "loginAliasId=" . $user;
		$queries["loginAuonePwd"] = "loginAuonePwd=" . $pass;
		$queries["loginStateHold"] = "loginStateHold="; // 「ログイン状態を維持する」チェックボックスをオフに設定する
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = auwallet_update_cookie($head, $cookie);
				list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
			}
		}
	}
}

if(strpos($body, "2段階認証") !== false) {
	// 2段階認証する
	
	$ps = parse_tag($body, "p");
	$shitsumon = "";
	foreach($ps as $p) {
		if($p["class"] == "f_14") {
			$shitsumon = trim(strip_tags($p["innerHTML"]));
			// breakしない
		}
	}
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "form_confirm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = auwallet_update_cookie($head, $cookie);
			list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		}
	}
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "hidden";
	$resp["additional"] = $shitsumon;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $user, $pass, ENV_STR_SESSION_PADDING)));
	
	$mfachallengetrnrs = "";
	$mfachallengetrnrs .= "<MFACHALLENGETRNRS>";
	$mfachallengetrnrs .= "<MFACHALLENGERS>";
	$mfachallengetrnrs .= "<MFACHALLENGE>";
	$mfachallengetrnrs .= "<MFAPHRASEID>" . $resp["aid"] . "</MFAPHRASEID>";
	$mfachallengetrnrs .= "<MFAPHRASELABEL>" . $resp["additional"] . "</MFAPHRASELABEL>";
	$mfachallengetrnrs .= "</MFACHALLENGE>";
	$mfachallengetrnrs .= "</MFACHALLENGERS>";
	$mfachallengetrnrs .= "</MFACHALLENGETRNRS>";
	
	$resp["ofx"] = generate_ofx($resp["status"], $mfachallengetrnrs, $resp["sesscookie"], $resp["accesskey"]);
} else if(strpos($body, "現在メンテナンスを行っております") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "配送状況") !== false) {
	// ログイン後の画面が通常と異なる場合
	$resp["status"] = ENV_NUM_STATUS_CAUTION;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "メニュー一覧") === false) {
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
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	$account["acctname"] = $settings["name"] . ENV_CHR_CONCATENATOR . $user;
	$account["acctid"] = $user;
	
	// 支店番号を取得する
	$account["branchid"] = "0";
	
	// 残高を取得する
	$dls = parse_tag($body, "dl");
	foreach($dls as $dl) if(strpos($dl["innerHTML"], "残高") !== false) {
		$ps = parse_tag($dl["innerHTML"], "p");
		$account["balance"] = parse_amount(trim(strip_tags($ps[0]["innerHTML"])));
		break;
	}
	
	// メニュー一覧画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(strip_tags($a["innerHTML"]) == "メニュー一覧") {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// カード情報照会/設定画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "カード情報照会/設定");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		
		// 口座番号を取得する
		$tds = parse_tag($body, "td");
		if(count($tds) > 2) $account["acctid"] = trim(strip_tags($tds[2]["innerHTML"]));
	}
	
	// お買い物履歴照会画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(trim(strip_tags($a["innerHTML"])) == "お買い物履歴照会") {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$account["details"] = array();
	
	$csv = "";
	$body_old = $body;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 今月のCSVファイルを取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "download_form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["menuFormDto.history.selectedDlBtnName"] = "menuFormDto.history.selectedDlBtnName=prepaid";
		
		$forms[$c]["action"] = "history_print.html";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		
		if(strpos($head, "Content-Type: text/html") === false) $csv = mb_convert_string($body);
	}
	
	$body = $body_old;
	
	// 前月の画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "history_form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["menuFormDto.viewDate"] = "menuFormDto.viewDate=" . substr(ENV_STR_DATE_PASTDAY, 0, 6);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	}
	
	$body_old = $body;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 前月のCSVファイルを取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "download_form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["menuFormDto.history.selectedDlBtnName"] = "menuFormDto.history.selectedDlBtnName=prepaid";
		
		$forms[$c]["action"] = "history_print.html";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		
		if(strpos($head, "Content-Type: text/html") === false) $csv .= mb_convert_string($body);
	}
	
	$body = $body_old;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// チャージ履歴画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "チャージ履歴はこちら");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	}
	
	// 今月のチャージ履歴を取得する
	$divs = parse_tag($body, "div", true); // 再帰的に取得する
	foreach($divs as $div) if($div["class"] == "accordion") {
		$memo = "";
		$amount = "";
		$date = "";
		$summary = "";
		
		$tds = parse_tag($div["innerHTML"], "td");
		if(count($tds) >= 4) {
			$memo = $tds[2]["innerHTML"];
			$amount = parse_amount($tds[3]["innerHTML"]);
		}
		
		$dds = parse_tag($div["innerHTML"], "dd");
		if(count($dds) >= 2) {
			$date = $dds[0]["innerHTML"];
			$summary = $dds[1]["innerHTML"];
		}
		
		if($date != "" && $summary != "" && $amount != "" && $memo != "") $csv .= implode(",", array("0", $date, $summary, "チャージ", $amount, "", $memo, "", "", "")) . "\r\n";
	}
	
	// 前月の画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "history_form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["menuFormDto.viewDate"] = "menuFormDto.viewDate=" . substr(ENV_STR_DATE_PASTDAY, 0, 6);
		$forms[$c]["action"] = "charge_history.html";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	}
	
	// 前月のチャージ履歴を取得する
	$divs = parse_tag($body, "div", true); // 再帰的に取得する
	foreach($divs as $div) if($div["class"] == "accordion") {
		$memo = "";
		$amount = "";
		$date = "";
		$summary = "";
		
		$tds = parse_tag($div["innerHTML"], "td");
		if(count($tds) >= 4) {
			$memo = $tds[2]["innerHTML"];
			$amount = parse_amount($tds[3]["innerHTML"]);
		}
		
		$dds = parse_tag($div["innerHTML"], "dd");
		if(count($dds) >= 2) {
			$date = $dds[0]["innerHTML"];
			$summary = $dds[1]["innerHTML"];
		}
		
		if($date != "" && $summary != "" && $amount != "" && $memo != "") $csv .= implode(",", array("0", $date, $summary, "チャージ", $amount, "", $memo, "", "", "")) . "\r\n";
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	$details = array();
	$j = 0;
	$lines = array_reverse(parse_csv($csv));
	foreach($lines as $line) {
		$c = count($line);
		if($c >= 9 && $line[0] != "" && $line[1] != "利用日時") {
			// $details[$j]["date"] = parse_date($line[1]);
			$details[$j]["date"] = $line[1];
			$details[$j]["summary"] = $line[2];
			$details[$j]["amount"] = (string)(($line[3] == "支払"? -1: 1) * (double)parse_amount($line[4]));
			$details[$j]["memo"] = ($line[6] != ""? $line[6]: ENV_STR_OFX_MEMO);
			
			if($line[2] == "キャンペーン" && $line[3] == "キャンペーン" && $details[$j]["amount"] == "0" && $line[6] != "") {
				$cs = explode(":", $line[6]);
				$details[$j]["amount"] = parse_amount($cs[count($cs) - 1]);
			}
			$j++;
		}
	}
	asort($details);
	$account["details"] = $details;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "ログアウト");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = auwallet_update_cookie($head, $cookie);
		list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = auwallet_update_cookie($head, $cookie);
			list($head, $body) = auwallet_http11($method, $uris, $query, $cookie);
		}
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
	$bankmsgsrsv1 .= auwallet_parse_details($account);
	$bankmsgsrsv1 .= "</STMTRS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "</STMTTRNRS>";
	$bankmsgsrsv1 .= "\r\n";
	
	$bankmsgsrsv1 .= "</BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	if(ENV_BOOL_ADD_RISKBASE == true) {
		// 次回ログイン時に2段階認証を行わない
		$resp["token"] = auwallet_get_token("DVCK", $cookie);
	}
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	$resp["ofx"] = generate_ofx($resp["status"], $bankmsgsrsv1);
}
return $resp;

// HTTP/1.1
function auwallet_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.auone\.jp$/", $uris["host"]) > 0 || preg_match("/\.au\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function auwallet_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID", "BIGipServerCM-US-PC-VWC", "DVOTKT", "BIGipServerCM-US-PC-TF", "BIGipServerCM-US-PC-NTF", "DVCK", "DTKT", "VTKT", "ACST", "CCATRCT", "CCADAST"), parse_header($head, "set-cookie"), $cookie);
}

function auwallet_get_token($name, $cookie) {
	$ret = "";
	
	$kvs = explode(";", $cookie);
	foreach($kvs as $kv) {
		list($k, $v) = explode("=", $kv, 2);
		$k = trim($k);
		if($k == $name) {
			$ret = $k . "=" . trim($v);
			break;
		}
	}
	return $ret;
}

function auwallet_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	$dtstart = ENV_STR_DATE_PASTDAY;
	foreach($account["details"] as $line) {
		$line["date"] = parse_date($line["date"]);
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
