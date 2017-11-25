<?php
/*
esaifu.inc: e-さいふのHTMLよりOFXファイルを生成する
Copyright (C) 2013-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["user_id"];
$pass = $ofxforms["password"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["image_key"]), "Shift_JIS", "UTF-8"));
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];

$resp = array();
$ofx = "";

if($sesscookie != "") list($cookie, $dummy) = explode("\t", sess_decode($sesscookie));
if($accesskey != "") {
	list($ssid, $method, $uri, $query, $user, $dummy) = explode("\t", sess_decode($accesskey), 6);
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
	list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = esaifu_update_cookie($head, $cookie);
			list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
		}
	}
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "会員ログイン）") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = esaifu_update_cookie($head, $cookie);
		list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 画像認証の画像を取得する
	$imgs = parse_tag($body, "img");
	$c = parse_tag_search($imgs, "id", "siimage");
	if($c != -1) {
		$imguris = parse_uri($imgs[$c]["src"], $uris);
		list($imghead, $imgbody) = esaifu_http11($method, $imguris, $query, $cookie, $uris["scheme"] . "://" . $uris["host"] . $uris["path"] . $query);
		$imgsrc = "data:image/png;base64," . base64_encode($imgbody);
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	if(count($forms) > 0) {
		$inputs = parse_tag($forms[0]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["user_id"] = "user_id=" . $user;
		$queries["password"] = "password=" . $pass;
		$queries["image_key"] = "image_key=";
		
		$method = $forms[0]["method"];
		$uris = parse_uri($forms[0]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = esaifu_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("image_key=", "image_key=" . $auth, $query);
	list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "form1");
	if($c != -1) {
		$scripts = parse_tag($body, "script");
		foreach($scripts as $script) if(preg_match("/([\'\"])([^\\1]*?)\\1/i", $script["innerHTML"], $matches) > 0) {
			$forms[$c]["action"] = $matches[2];
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			// $queryは一つ前のものを使い回す
			// $query = implode("&", $queries);
			$cookie = esaifu_update_cookie($head, $cookie);
			list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
			break;
		}
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = esaifu_update_cookie($head, $cookie);
			list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
		}
	}
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "image_key";
	$resp["additional"] = $imgsrc;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $user, ENV_STR_SESSION_PADDING)));
	
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
} else if(strpos($body, "eさいふ | メンテナンス中") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回のご利用日時") === false) {
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
	$ps = parse_tag($body, "p");
	$c = parse_tag_search($ps, "class", "price");
	if($c != -1) $account["balance"] = parse_amount(strip_tags($spans[0]["innerHTML"]));
	
	// 履歴画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "履歴");
	if($c != 1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = esaifu_update_cookie($head, $cookie);
		list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
	}
	
	// 履歴画面（1ページ目）を取得する
	$forms = parse_tag($body, "form");
	if(count($forms) > 0) {
		$queries = array();
		$inputs = parse_tag($forms[0]["innerHTML"], "input");
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$selects = parse_tag($forms[0]["innerHTML"], "select");
		foreach($selects as $select) {
			$options = parse_tag($select["innerHTML"], "option");
			foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
		}
		$queries["user_year_from"] = "user_year_from=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 0, 4);
		$queries["user_month_from"] = "user_month_from=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["user_day_from"] = "user_day_from=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 6, 2);
		$queries["user_year_to"] = "user_year_to=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 0, 4);
		$queries["user_month_to"] = "user_month_to=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["user_day_to"] = "user_day_to=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 6, 2);
		
		$method = $forms[0]["method"];
		$uris = parse_uri($forms[0]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = esaifu_update_cookie($head, $cookie);
		list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
	}
	
	$tbody = "";
	
	// 無限ループする
	while(true) {
		// 明細を取得する
		$tables = parse_tag($body, "table");
		$c = parse_tag_search($tables, "class", "only_pc");
		if($c != -1) {
			$tbodys = parse_tag($tables[$c]["innerHTML"], "tbody");
			if(count($tbodys) > 0) $tbody .= $tbodys[0]["innerHTML"];
		}
		
		// 履歴画面（2ページ目以降）を取得する
		$as = parse_tag($body, "a");
		$found = false;
		foreach($as as $a) {
			if(trim(strip_tags($a["innerHTML"])) == "次へ") {
				$method = "GET";
				$uris = parse_uri($a["href"], $uris);
				$query = "";
				$cookie = esaifu_update_cookie($head, $cookie);
				list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
				
				$found = true;
				break;
			}
		}
		
		// 見つからない場合、ループを抜ける
		if($found == false) break;
	}
	
	$account["details"] = esaifu_get_details($tbody);
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(strip_tags($a["innerHTML"]) == "ログアウト") {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = esaifu_update_cookie($head, $cookie);
		list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = esaifu_update_cookie($head, $cookie);
			list($head, $body) = esaifu_http11($method, $uris, $query, $cookie);
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
	$bankmsgsrsv1 .= esaifu_parse_details($account);
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
function esaifu_http11($method, $uris, $query = "", $cookie = "", $referer = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.mun-prepaid\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie, true, ENV_PRODUCT_UA, "", $referer);
	return explode("\r\n\r\n", $ret, 2);
}

function esaifu_update_cookie($head, $cookie) {
	return update_cookie(array("cookie_use_ok", "sslCookieUseOk", "vp_partner_id", "vp_session", "Nicos_pmk"), parse_header($head, "set-cookie"), $cookie);
}

function esaifu_get_details($body) {
	$rets = array();
	$i = 0;
	$trs = parse_tag($body, "tr");
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 7) {
			// 利用日付を取得する
			$dt = explode(" ", $tds[1]["innerHTML"]);
			$rets[$i]["date"] = parse_date(trim(strip_tags($dt[0])));
			
			// ご利用種別、ご利用内容、およびお取引IDを取得する
			$buf = trim(strip_tags($tds[2]["innerHTML"]));
			switch($buf) {
			case "ご利用（予約）":
				$name = $buf;
				$memo = implode(ENV_CHR_CONCATENATOR, array(str_replace("カード下4桁 ", "", trim(strip_tags($tds[3]["innerHTML"]))), trim(strip_tags($tds[6]["innerHTML"]))));
				break;
			case "ご利用（確定）":
				list($name, $memo) = explode("<br>", $tds[3]["innerHTML"], 2);
				$name = trim(strip_tags($name));
				$memo = implode(ENV_CHR_CONCATENATOR, array(str_replace("カード下4桁 ", "", trim(strip_tags($memo))), trim(strip_tags($tds[6]["innerHTML"]))));
				break;
			default:
				$name = trim(strip_tags($tds[3]["innerHTML"]));
				$memo = $buf;
				break;
			}
			$rets[$i]["summary"] = $name;
			$rets[$i]["memo"] = $memo;
			
			// ご利用金額を取得する
			$rets[$i]["amount"] = parse_amount(strip_tags($tds[4]["innerHTML"]));
			
			$i++;
		}
	}
	
	return $rets;
}

function esaifu_parse_details($account) {
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
