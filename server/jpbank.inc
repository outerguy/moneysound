<?php
/*
jpbank.inc: ゆうちょ銀行のHTMLよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = str_replace("-", "", $ofxforms["okyakusamaBangou"]);
$pass = $ofxforms["loginPassword"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["aikotoba"]), "Shift_JIS", "UTF-8"));
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];

$resp = array();
$sid = 0;
$method = "";
$query = "";
$cookie = "";
$head = "";
$body = "";

if($sesscookie != "") list($cookie, $dummy) = explode("\t", sess_decode($sesscookie));
if($accesskey != "") {
	list($ssid, $method, $uri, $query, $pass, $dummy) = explode("\t", sess_decode($accesskey), 6);
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
	list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ゆうちょダイレクト　ログイン") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
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
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
	
	// お客さま番号を入力する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) switch($input["value"]) {
		case "次へ":
			$dcs = jpbank_parse_dcrequest($input["onclick"]);
			// breakしない
		default:
			if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			break;
		}
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["okyakusamaBangou1"] = "okyakusamaBangou1=" . substr($user, 0, 4);
		$queries["okyakusamaBangou2"] = "okyakusamaBangou2=" . substr($user, 4, 4);
		$queries["okyakusamaBangou3"] = "okyakusamaBangou3=" . substr($user, 8);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "合言葉入力１") !== false) {
	// 合言葉を入力する（１）
	$dds = parse_tag($body, "dd");
	if(count($dds) > 0) $shitsumon = trim(strip_tags($dds[0]["innerHTML"]));
	
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "次へ");
	if($c != -1) $dcs = jpbank_parse_dcrequest($as[$c]["onclick"]);
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["aikotoba"] = "aikotoba=";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
	}
	
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("aikotoba=", "aikotoba=" . $auth, $query);
	list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "合言葉入力２") !== false) {
	// 合言葉を入力する（２）
	$dds = parse_tag($body, "dd");
	if(count($dds) > 0) $shitsumon = trim(strip_tags($dds[0]["innerHTML"]));
	
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "次へ");
	if($c != -1) $dcs = jpbank_parse_dcrequest($as[$c]["onclick"]);
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["aikotoba"] = "aikotoba=";
		if(isset($queries["riyouKankyouTourokuFuyouUmu"]) == true) {
			if(ENV_BOOL_ADD_RISKBASE == true) {
				// 次回ログイン時に合言葉を入力しない
				unset($queries["riyouKankyouTourokuFuyouUmu"]);
			} else {
				// 次回ログイン時に合言葉を入力する
				$queries["riyouKankyouTourokuFuyouUmu"] = "riyouKankyouTourokuFuyouUmu=1";
			}
		}
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
	}
	
	$sid = 2;
	$head = "";
	$body = "";
} else if($sid == 2) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("aikotoba=", "aikotoba=" . $auth, $query);
	list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "ログインパスワード入力") !== false) {
	// ログインパスワードを入力する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) switch($input["value"]) {
		case "ログイン":
			$dcs = jpbank_parse_dcrequest($input["onclick"]);
			// breakしない
		default:
			if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			break;
		}
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["loginPassword"] = "loginPassword=" . $pass;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
}

// お知らせ専用画面が表示される場合、次の画面を取得する
$as = parse_tag($body, "a");
$c = parse_tag_search($as, "innerHTML", "ダイレクトトップ");
if($c != -1) $dcs = jpbank_parse_dcrequest($as[$c]["onclick"]);
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "simpleTransitionForm");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$forms[$c]["action"] = $dcs["path"];
	if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = jpbank_update_cookie($head, $cookie);
	list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "aikotoba";
	$resp["additional"] = $shitsumon;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $pass, ENV_STR_SESSION_PADDING)));
	
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
} else if(strpos($body, "ご利用時間外のためお取扱いただけません") !== false || strpos($body, "ただいまサービスを行っておりません") !== false) {
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
	$delta = 1988;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 口座情報を取得する
	$divs = parse_tag($body, "div", true); // 再帰的に取得する
	$c = parse_tag_search($divs, "class", "col w45");
	if($c != -1) {
		if(preg_match("/\((.+?)" . preg_quote("：") . "(.+?)&emsp;([0-9]+)\)/", $divs[$c]["innerHTML"], $matches) > 0) {
			$accttype = jpbank_getaccttype($matches[1]);
			$branchid = str_replace(array("〇", "一", "二", "三", "四", "五" ,"六" ,"七", "八", "九", "店"), array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ""), $matches[2]);
			$acctid = $matches[3];
			$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $matches[2], $matches[1]));
		}
	}
	
	$c = parse_tag_search($divs, "class", "col w55");
	if($c != -1) $account["balance"] = parse_amount(trim(strip_tags($divs[$c]["innerHTML"])));
	
	// 入出金明細照会画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "入出金明細照会");
	if($c != -1) $dcs = jpbank_parse_dcrequest($as[$c]["onclick"]);
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "simpleTransitionForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
	
	// 入出金明細を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) {
			if($input["type"] == "button" && $input["onclick"] != "") $dcs = jpbank_parse_dcrequest($input["onclick"]);
			if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		}
		$selects = parse_tag($forms[$c]["innerHTML"], "select");
		foreach($selects as $select) {
			$options = parse_tag($select["innerHTML"], "option");
			// foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
			foreach($options as $option) {
				if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
				// 西暦と和暦の差を取得する
				if(preg_match("/[^0-9]+([0-9]+)\(([0-9]+)\)/", $option["innerHTML"], $matches) > 0) $delta = (integer)$matches[2] - (integer)$matches[1];
			}
		}
		
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["shoukaiTaishouSentaku"] = "shoukaiTaishouSentaku=01"; // 01=入出金
		$queries["shoukaiHaniSentaku"] = "shoukaiHaniSentaku=01"; // 00=日付を指定
		$queries["nyuushukkinShoukaiHaniKaishiNen"] = "nyuushukkinShoukaiHaniKaishiNen=" . substr(ENV_STR_DATE_PASTDAY, 0, 4);
		$queries["shiteiKanouKaishiTsukiId"] = "shiteiKanouKaishiTsukiId=" . substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["shiteiKanouKaishiHiId"] = "shiteiKanouKaishiHiId=" . substr(ENV_STR_DATE_PASTDAY, 6, 2);
		$queries["nyuushukkinShoukaiHaniShuuryouNen"] = "nyuushukkinShoukaiHaniShuuryouNen=" . substr(ENV_STR_DATE_TODAY, 0, 4);
		$queries["shiteiKanouShuuryouTsukiId"] = "shiteiKanouShuuryouTsukiId=" . substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["shiteiKanouShuuryouHiId"] = "shiteiKanouShuuryouHiId=" . substr(ENV_STR_DATE_TODAY, 6, 2);
		$queries["kensuuSentaku"] = "kensuuSentaku=100"; // 100=表示件数
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		}
	}
	
	// 明細を取得する
	$account["details"] = array();
	$tables = parse_tag($body, "table");
	$c = parse_tag_search($tables, "class", "tblTy91 tblTy06");
	if($c != -1) {
		$tbodys = parse_tag($tables[$c]["innerHTML"], "tbody");
		if(count($tbodys) > 0) $account["details"] = jpbank_get_details($tbodys[0]["innerHTML"], $delta);
	}
	
	/*
	$body_old = $body;
	
	// CSVファイルをダウンロードする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "ダウンロード");
	if($c != -1) $dcs = jpbank_parse_dcrequest($as[$c]["onclick"]);
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
		
		if(strpos($head, "Content-Type: text/html") === false) {
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
			$bankmsgsrsv1 .= "<BRANCHID>" . $branchid . "</BRANCHID>";
			$bankmsgsrsv1 .= "<ACCTID>" . $acctid . "</ACCTID>";
			$bankmsgsrsv1 .= "<ACCTTYPE>" . $accttype . "</ACCTTYPE>";
			$bankmsgsrsv1 .= "</BANKACCTFROM>";
			$bankmsgsrsv1 .= "\r\n";
			$bankmsgsrsv1 .= jpbank_parse_csv($body, $account);
			$bankmsgsrsv1 .= "</STMTRS>";
			$bankmsgsrsv1 .= "\r\n";
			$bankmsgsrsv1 .= "</STMTTRNRS>";
			$bankmsgsrsv1 .= "\r\n";
		}
	}
	
	$body = $body_old;
	*/
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if($a["innerHTML"] == "ログアウト" && $a["onclick"] != "") {
		$dcs = jpbank_parse_dcrequest($a["onclick"]);
		break;
	}
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "simpleTransitionForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jpbank_update_cookie($head, $cookie);
		list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = jpbank_update_cookie($head, $cookie);
			list($head, $body) = jpbank_http11($method, $uris, $query, $cookie);
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
	$bankmsgsrsv1 .= "<BRANCHID>" . $branchid . "</BRANCHID>";
	$bankmsgsrsv1 .= "<ACCTID>" . $acctid . "</ACCTID>";
	$bankmsgsrsv1 .= "<ACCTTYPE>" . $accttype . "</ACCTTYPE>";
	$bankmsgsrsv1 .= "</BANKACCTFROM>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= jpbank_parse_details($account);
	$bankmsgsrsv1 .= "</STMTRS>";
	$bankmsgsrsv1 .= "\r\n";
	$bankmsgsrsv1 .= "</STMTTRNRS>";
	$bankmsgsrsv1 .= "\r\n";
	
	$bankmsgsrsv1 .= "</BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	$resp["ofx"] = generate_ofx($resp["status"], $bankmsgsrsv1);
	
	/*
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	if(strpos($bankmsgsrsv1, "<BANKTRANLIST>") === false) {
		// 明細が存在しない場合
		$resp["ofx"] = generate_ofx($resp["status"]);
	} else {
		// 明細が存在する場合
		$resp["ofx"] = generate_ofx($resp["status"], $bankmsgsrsv1);
	}
	*/
}
return $resp;

// HTTP/1.1
function jpbank_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.jp\-bank\.japanpost\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function jpbank_update_cookie($head, $cookie) {
	return update_cookie(array("BROWSER_TOKEN", "JSESSIONID", "PMData"), parse_header($head, "set-cookie"), $cookie);
}

// 預金種類よりACCTTYPEを決定する
function jpbank_getaccttype($str) {
	$ret = ENV_STR_ACCTTYPE_SAVINGS;
	switch($str) {
	case "当座":
		$ret = ENV_STR_ACCTTYPE_CHECKING;
		break;
	case "貯蓄":
	case "普通":
	default:
		break;
	}
	return $ret;
}

function jpbank_parse_dcrequest($str) {
	$ret = array();
	if(preg_match("/dcRequest\(([^,]+),([^,]+),[^\(]+\(([^\)]+)\),[^\(]+\(([^\)]+)\),([^,]+)\);/i", $str, $matches) > 0) {
		$ret["path"] = jpbank_delete_quote($matches[2]);
		$ret["kvs"] = array();
		$ks = explode(",", $matches[3]);
		$vs = explode(",", $matches[4]);
		for($i = 0; $i < count($ks); $i++) {
			$k = jpbank_delete_quote($ks[$i]);
			$v = jpbank_delete_quote($vs[$i]);
			$ret["kvs"][$k] = $k . "=" . $v;
		}
	} else if(preg_match("/dcRequest\(([^,]+),([^,]+),([^,]+),([^,]+),([^,]+)\);/i", $str, $matches) > 0) {
		$ret["path"] = jpbank_delete_quote($matches[2]);
	}
	return $ret;
}

function jpbank_delete_quote($str) {
	return preg_replace("/^.*?([\"\'])(.*?)\\1.*?$/", "\\2", $str);
}

function jpbank_get_details($body, $delta) {
	$rets = array();
	$i = 0;
	$trs = parse_tag($body, "tr");
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 5) {
			// 取引年月日を取得する
			$ymds = explode("-", trim(strip_tags($tds[0]["innerHTML"])));
			$rets[$i]["date"] = parse_date(implode("-", array((string)((integer)$ymds[0] + $delta), $ymds[1], $ymds[2])));
			
			// 預入／支払内容を取得する
			$rets[$i]["summary"] = str_replace("&nbsp;", "", trim(strip_tags($tds[3]["innerHTML"])));
			
			// 預入／支払金額を取得する
			$rets[$i]["amount"] = (string)((integer)parse_amount($tds[1]["innerHTML"]) - (integer)parse_amount($tds[2]["innerHTML"]));
			
			$i++;
		}
	}
	
	return $rets;
}

function jpbank_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	foreach($account["details"] as $line) {
		$cd = array();
		$trntypes = explode("　", $line["summary"], 2);
		switch($trntypes[0]) {
		case "振込":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
			break;
		case "振替":
		case "自払":
		case "割賦":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
			break;
		case "送金":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_XFER;
			break;
		case "払込":
		case "払出":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CASH;
			break;
		case "会費":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_REPEATPMT;
			break;
		/*
		case "通　帳": // 要検討
		*/
		case "カード":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
			break;
		/*
		case "料　金": // 要検討
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_FEE;
			break;
		*/
		case "受取利子":
		case "利子":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
			break;
		case "税金":
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
			break;
		default:
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
			break;
		}
		
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
		$cd["MEMO"] = ENV_STR_OFX_MEMO;
		
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

/*
function jpbank_parse_csv($str, $account) {
	$ret = "";
	$lines = parse_csv(mb_convert_string($str));
	$cds = array();
	$cd_date = "";
	foreach($lines as $line) {
		$cd = array();
		if(count($line) == 8 && $line[0] != "取引日") {
			switch($line[4]) {
			case "振込":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
				break;
			case "振替":
			case "自払":
			case "割賦":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
				break;
			case "送金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_XFER;
				break;
			case "払込":
			case "払出":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CASH;
				break;
			case "会費":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_REPEATPMT;
				break;
			case "通　帳":
			case "カード":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
				break;
			case "料　金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_FEE;
				break;
			case "受取利子":
			case "利子":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
				break;
			case "税金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
				break;
			default:
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
				break;
			}
			// 日付を取得する
			$cd["DTPOSTED"] = $line[0];
			// トランザクション番号を生成する
			$cd["FITID"] = substr($line[1], 0, 8) . sprintf("%04d", (integer)substr($line[1], 8, 2)) . "000" . substr($line[1], 10, 5);
			// 摘要を取得する
			$cd["NAME"] = $line[4] . ($line[5] != ""? ENV_CHR_CONCATENATOR . $line[5]: "");
			// 金額を取得する
			$cd["TRNAMT"] = (string)((double)parse_amount($line[2]) - (double)parse_amount($line[3]));
			$cd["MEMO"] = ENV_STR_OFX_MEMO;
			
			array_push($cds, $cd);
			$cd_date = $cd["DTPOSTED"];
		}
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
*/

?>
