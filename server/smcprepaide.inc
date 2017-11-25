<?php
/*
smcprepaide.inc: 三井住友VISAプリペイドeのHTMLよりOFXファイルを生成する
Copyright (C) 2014-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["loginId"];
$pass = $ofxforms["loginPassword"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["authCode"]), "Shift_JIS", "UTF-8"));
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];

$resp = array();
$ofx = "";

if($sesscookie != "") list($cookie, $dummy) = explode("\t", sess_decode($sesscookie));
if($accesskey != "") {
	list($ssid, $method, $uri, $query, $svid, $dummy) = explode("\t", sess_decode($accesskey), 6);
	$sid = (integer)$ssid;
}

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

if($sid == 0) {
	// ホーム画面を取得する
	$method = "GET";
	$uris = parse_uri($settings["home"]);
	$query = "";
	$cookie = "";
	list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = smcprepaide_update_cookie($head, $cookie);
			list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		}
	}
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログイン") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = smcprepaide_update_cookie($head, $cookie);
		list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// リダイレクトする
	$scripts = parse_tag($body, "script");
	foreach($scripts as $script) if(preg_match("/window\.location[\s\t]*=[\s\t]*\"(.*?)\";/i", $script["innerHTML"], $matches) > 0) {
		$method = "GET";
		$uris = parse_uri($matches[1], $uris);
		$query = "";
		$cookie = smcprepaide_update_cookie($head, $cookie);
		list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// svidを取得する
	$svid = "";
	$scripts = parse_tag($body, "script");
	foreach($scripts as $script) if(preg_match("/svid[\s\t]*=[\s\t]*\"(.*?)\";/i", $script["innerHTML"], $matches) > 0) {
		$svid = $matches[1];
		break;
	}
	
	// 画像認証の画像を取得する
	$imgs = parse_tag($body, "img");
	$c = parse_tag_search($imgs, "id", "captchaImage");
	if($c != -1) {
		$imguris = parse_uri($imgs[$c]["src"], $uris);
		list($imghead, $imgbody) = smcprepaide_http11($method, $imguris, $query, $cookie, $uris["scheme"] . "://" . $uris["host"] . $uris["path"] . $query);
		$imgsrc = "data:image/jpeg;base64," . base64_encode($imgbody);
		$cookie = smcprepaide_update_cookie($imghead, $cookie);
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "WB0102SC01Form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["loginId"] = "loginId=" . $user;
		$queries["loginPassword"] = "loginPassword=" . $pass;
		$queries["authCode"] = "authCode=";
		// if(isset($queries["forward_login"]) == true) unset($queries["forward_login"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("authCode=", "authCode=" . $auth, $query);
	list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "authCode";
	$resp["additional"] = $imgsrc;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $svid, ENV_STR_SESSION_PADDING)));
	
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
} else if(strpos($body, "定例または臨時メンテナンスのため") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "重要なお知らせがあります") !== false) {
	// ログイン後の画面が通常と異なる場合
	$resp["status"] = ENV_NUM_STATUS_CAUTION;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回ログイン") === false) {
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
	$accounts = array();
	
	// 口座数分ループする
	$forms = parse_tag($body, "form");
	for($c = 1; $c < count($forms); $c++) {
		// 登録カード詳細画面を取得する
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		// if(isset($queries["forward_toWB3202SC01"]) == true) unset($queries["forward_toWB3202SC01"]); // 詳細
		if(isset($queries["forward_toWB2301SC01"]) == true) unset($queries["forward_toWB2301SC01"]); // チャージ
		if(isset($queries["forward_toWB2601SC01"]) == true) unset($queries["forward_toWB2601SC01"]); // ロック
		if(isset($queries["forward_toWB2602SC01"]) == true) unset($queries["forward_toWB2602SC01"]); // ロック解除
		if(isset($queries["forward_toWB3301SC01"]) == true) unset($queries["forward_toWB3301SC01"]); // 利用履歴照会
		if(isset($queries["forward_toWB2901SC01"]) == true) unset($queries["forward_toWB2901SC01"]); // 暗証番号設定
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = smcprepaide_update_cookie($head, $cookie);
		list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		
		// 利用履歴照会画面を取得する
		$forms2 = parse_tag($body, "form");
		$inputs = parse_tag($forms2[0]["innerHTML"], "input");
		
		// 口座名（ニックネーム）を取得する
		$d = parse_tag_search($inputs, "name", "cardNickname");
		if($d != -1) $accounts[$c - 1]["acctname"] = $settings["name"] . ENV_CHR_CONCATENATOR . $inputs[$d]["value"];
		
		// 支店番号を取得する
		$accounts[$c - 1]["branchid"] = "0";
		
		// 口座番号を取得する
		$d = parse_tag_search($inputs, "name", "vcn");
		if($d != -1) $accounts[$c - 1]["acctid"] = $inputs[$d]["value"];
		
		// 残高を取得する
		$d = parse_tag_search($inputs, "name", "chargeBalance");
		if($d != -1) $accounts[$c - 1]["balance"] = $inputs[$d]["value"];
		
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		unset($queries["forward_toWB2301SC01"]); // チャージ
		// if(isset($queries["forward_toWB3301SC01"]) == true) unset($queries["forward_toWB3301SC01"]); // 利用履歴照会
		if(isset($queries["forward_toWB2601SC01"]) == true) unset($queries["forward_toWB2601SC01"]); // ご利用ロック設定・解除
		if(isset($queries["forward_toWB2701SC01"]) == true) unset($queries["forward_toWB2701SC01"]); // カード名称の変更
		if(isset($queries["forward_toWB2901SC01"]) == true) unset($queries["forward_toWB2901SC01"]); // 暗証番号設定
		if(isset($queries["forward_toWB3201SC01"]) == true) unset($queries["forward_toWB3201SC01"]); // 戻る
		
		$method = $forms2[0]["method"];
		$uris = parse_uri($forms2[0]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = smcprepaide_update_cookie($head, $cookie);
		list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		
		$detail = "";
		
		do {
			$tbodys = parse_tag($body, "tbody");
			if(count($tbodys) > 1) $detail .= $tbodys[1]["innerHTML"];
			
			$as = parse_tag($body, "a");
			$d = parse_tag_search($as, "innerHTML", "次へ");
			if($d != -1) {
				$metod = "GET";
				$uris = parse_uri($as[$d]["href"], $uris);
				$query = "";
				$cookie = smcprepaide_update_cookie($head, $cookie);
				list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
			}
		} while($d != -1);
		
		if($detail != "") $accounts[$c - 1]["details"] = smcprepaide_get_details($detail);
		
		/*
		// カード一覧画面を取得する
		$as = parse_tag($body, "a");
		$c = parse_tag_search($as, "innerHTML", "カード一覧");
		if($c != -1) {
			$method = "GET";
			$uris = parse_uri($as[$c]["href"] . $svid, $uris);
			$query = "";
			$cookie = smcprepaide_update_cookie($head, $cookie);
			list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		}
		*/
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログアウト") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"] . $svid, $uris);
		$query = "";
		$cookie = smcprepaide_update_cookie($head, $cookie);
		list($head, $body) = smcprepaide_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$bankmsgsrsv1 = "";
	$bankmsgsrsv1 .= "<BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	foreach($accounts as $account) {
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
		$bankmsgsrsv1 .= "<ACCTTYPE>" . ENV_STR_ACCTTYPE_SAVINGS . "</ACCTTYPE>";
		$bankmsgsrsv1 .= "</BANKACCTFROM>";
		$bankmsgsrsv1 .= "\r\n";
		$bankmsgsrsv1 .= smcprepaide_parse_details($account);
		$bankmsgsrsv1 .= "</STMTRS>";
		$bankmsgsrsv1 .= "\r\n";
		$bankmsgsrsv1 .= "</STMTTRNRS>";
		$bankmsgsrsv1 .= "\r\n";
	}
	
	$bankmsgsrsv1 .= "</BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	if(strpos($bankmsgsrsv1, "<BANKTRANLIST>") === false) {
		// 明細が存在しない場合
		$resp["ofx"] = generate_ofx($resp["status"]);
	} else {
		// 明細が存在する場合
		$resp["ofx"] = generate_ofx($resp["status"], $bankmsgsrsv1);
	}
}
return $resp;

// HTTP/1.1
function smcprepaide_http11($method, $uris, $query = "", $cookie = "", $referer = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.smbc-card\.com$/", $uris["host"]) > 0 || preg_match("/\.vpass\.ne\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie, true, ENV_PRODUCT_UA, "", $referer);
	return explode("\r\n\r\n", $ret, 2);
}

function smcprepaide_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function smcprepaide_get_details($body) {
	$rets = array();
	$i = 0;
	$trs = array_reverse(parse_tag($body, "tr"));
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 9) {
			// ご利用日時を取得する
			$dt = explode(" ", trim(strip_tags($tds[2]["innerHTML"])));
			
			// 期限外の明細を除外する
			if(ENV_STR_DATE_PASTDAY > parse_date($dt[0])) continue;
			
			$rets[$i]["date"] = parse_date($dt[0]);
			
			// 支払区分、ご利用店名、ご入金額、およびご利用金額を取得する
			$buf = trim(strip_tags($tds[3]["innerHTML"]));
			switch($buf) {
			case "仮発行番号入力":
			case "返金":
			case "チャージ": // debug
				$name = $buf;
				$memo = trim(strip_tags($tds[8]["innerHTML"]));
				$amount = parse_amount(trim(strip_tags($tds[5]["innerHTML"])));
				break;
			case "利用":
			default:
				$name = trim(strip_tags($tds[4]["innerHTML"]));
				$memo = trim(strip_tags($tds[1]["innerHTML"]));
				$memo2 = trim(strip_tags($tds[8]["innerHTML"]));
				if($name == "") $name = $buf;
				if($memo2 != "") $memo .= ENV_CHR_CONCATENATOR . $memo2;
				$amount = "-" . parse_amount(trim(strip_tags($tds[6]["innerHTML"])));
				break;
			}
			$rets[$i]["summary"] = $name;
			$rets[$i]["memo"] = ($memo != ""? $memo: ENV_STR_OFX_MEMO);
			
			// ご利用金額を取得する
			$rets[$i]["amount"] = $amount;
			
			$i++;
		}
	}
	
	return $rets;
}

function smcprepaide_parse_details($account) {
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
