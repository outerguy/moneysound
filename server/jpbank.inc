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
	
	// より安心してご利用いただくにあたってのご注意画面やゆうちょダイレクトリニューアルについて画面が表示される場合、改めてログイン画面を取得する
	$as = parse_tag($body, "a", true);
	foreach($as as $a) if(strpos($a["href"], "#") === false && (parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "上記を確認のうえ、ログイン画面に進む") != -1 || parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログイン画面に進む") != -1)) {
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
		// $queries["pm_fp"] = $pmfp;
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
	$trs = parse_tag($body, "tr");
	$bufs = array();
	foreach($trs as $tr) {
		$ths = parse_tag($tr["innerHTML"], "th");
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($ths) == 1 && count($tds) == 1 && trim(strip_tags($ths[0]["innerHTML"])) == "質問") {
			$shitsumon = trim(strip_tags($tds[0]["innerHTML"]));
			break;
		}
	}
	
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
		// $queries["pm_fp"] = $pmfp;
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
	$trs = parse_tag($body, "tr");
	$bufs = array();
	foreach($trs as $tr) {
		$ths = parse_tag($tr["innerHTML"], "th");
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($ths) == 1 && count($tds) == 1 && trim(strip_tags($ths[0]["innerHTML"])) == "質問") {
			$shitsumon = trim(strip_tags($tds[0]["innerHTML"]));
			break;
		}
	}
	
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
		// $queries["pm_fp"] = $pmfp;
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
		// $queries["pm_fp"] = $pmfp;
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
} else if(strpos($body, "メインメニュー") === false) {
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
	
	// 口座情報を取得する
	if(preg_match("/ComInfoOutput\((.+?)\);/", $body, $matches) > 0) {
		$accs = parse_csv($matches[1]);
		$accttype = jpbank_getaccttype($accs[0][7]);
		$branchid = str_replace(array("〇", "一", "二", "三", "四", "五" ,"六" ,"七", "八", "九", "店"), array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ""), $accs[0][8]);
		$acctid = $accs[0][9];
		$account["balance"] = parse_amount($accs[0][12]);
		$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $accs[0][8], $accs[0][7]));
		
		// 現在時刻より西暦と和暦の差を取得する
		$delta = (preg_match("/[0-9]+/", $accs[0][17], $years) > 0? (integer)substr(ENV_STR_DATE_TODAY, 0, 4) - (integer)$years[0]: 0);
	}
	
	// 入出金明細照会画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "submitData");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$c = parse_tag_search($inputs, "name", "U030110"); // 入出金明細照会
		if($c != -1) $dcs = jpbank_parse_dcrequest($inputs[$c]["onclick"]);
	}
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
	
	// 入出金明細画面を取得する
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
		$selects = parse_tag($forms[$c]["innerHTML"], "select");
		foreach($selects as $select) {
			$options = parse_tag($select["innerHTML"], "option");
			foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
		}
		$forms[$c]["action"] = $dcs["path"];
		if(count($dcs["kvs"]) > 0) foreach($dcs["kvs"] as $k => $v) $queries[$k] = $v;
		$queries["shoukaiTaishouSentaku"] = "shoukaiTaishouSentaku=01"; // 01=入出金明細
		$queries["shoukaiHaniSentaku"] = "shoukaiHaniSentaku=00"; // 00=日付指定
		$queries["nyuushukkinShoukaiHaniWarekiKaishiNen"] = "nyuushukkinShoukaiHaniWarekiKaishiNen=" . (string)((integer)substr(ENV_STR_DATE_PASTDAY, 0, 4) - $delta);
		$queries["nyuushukkinShoukaiHaniWarekiKaishiTsuki"] = "nyuushukkinShoukaiHaniWarekiKaishiTsuki=" . (string)substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["nyuushukkinShoukaiHaniWarekiKaishiHi"] = "nyuushukkinShoukaiHaniWarekiKaishiHi=" . (string)substr(ENV_STR_DATE_PASTDAY, 6, 2);
		$queries["nyuushukkinShoukaiHaniWarekiShuuryouNen"] = "nyuushukkinShoukaiHaniWarekiShuuryouNen=" . (string)((integer)substr(ENV_STR_DATE_TODAY, 0, 4) - $delta);
		$queries["nyuushukkinShoukaiHaniWarekiShuuryouTsuki"] = "nyuushukkinShoukaiHaniWarekiShuuryouTsuki=" . (string)substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["nyuushukkinShoukaiHaniWarekiShuuryouHi"] = "nyuushukkinShoukaiHaniWarekiShuuryouHi=" . (string)substr(ENV_STR_DATE_TODAY, 6, 2);
		
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
	
	$details = array();
	$n = 0;
	
	do {
		// 明細を取得する
		$trs = parse_tag($body, "tr");
		foreach($trs as $tr) {
			$tds = parse_tag($tr["innerHTML"], "td");
			if(count($tds) == 5) {
				for($i = 0; $i < count($tds); $i++) $tds[$i]["innerHTML"] = str_replace("&nbsp;", "", trim(strip_tags($tds[$i]["innerHTML"])));
				$details[$n]["date"] = parse_date($tds[0]["innerHTML"]);
				$details[$n]["deposit"] = parse_amount($tds[1]["innerHTML"]);
				$details[$n]["withdrawals"] = parse_amount($tds[2]["innerHTML"]);
				$details[$n]["summary"] = ($tds[3]["innerHTML"] != ""? $tds[3]["innerHTML"]: ENV_STR_OFX_MEMO);
				$details[$n]["memo"] = ENV_STR_OFX_MEMO;
				$n++;
			}
		}
		
		$f = false;
		
		// 2ページ目以降を取得する
		$inputs = parse_tag($body, "input");
		$c = parse_tag_search($inputs, "value", "次を表示");
		if($c != -1) {
			$dcs = jpbank_parse_dcrequest($inputs[$c]["onclick"]);
			$f = true;
			
			$forms = parse_tag($body, "form");
			$c = parse_tag_search($forms, "name", "pageLinksBeanPageLinksForm");
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
		}
	} while($f == true);
	
	$account["details"] = $details;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "id", "LOGOUTBT"); // ログアウト
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
	
	$bankmsgsrsv1 = "";
	$bankmsgsrsv1 .= "<BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// 口座情報を生成する
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

function jpbank_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	foreach($account["details"] as $line) {
		$cd = array();
		if(mb_strpos($line["summary"], "振込") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
		} else if(mb_strpos($line["summary"], "振替") === 0 || mb_strpos($line["summary"], "自払") === 0 || mb_strpos($line["summary"], "割賦") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
		} else if(mb_strpos($line["summary"], "送金") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_XFER;
		} else if(mb_strpos($line["summary"], "払込") === 0 || mb_strpos($line["summary"], "払出") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CASH;
		} else if(mb_strpos($line["summary"], "会費") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_REPEATPMT;
		} else if(mb_strpos($line["summary"], "通　帳") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
		} else if(mb_strpos($line["summary"], "カード") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
		} else if(mb_strpos($line["summary"], "料　金") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_FEE;
		} else if(mb_strpos($line["summary"], "受取利子") === 0 || mb_strpos($line["summary"], "利子") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
		} else if(mb_strpos($line["summary"], "税金") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
		} else {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
		}
		// 日付を取得する
		$cd["DTPOSTED"] = $line["date"];
		// 通番を生成する
		if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
		// トランザクション番号を生成する
		$cd["FITID"] = $cd["DTPOSTED"] . sprintf("%04d", (integer)$account["id"]) . "000" . sprintf("%05d", $cd_num);
		// 摘要を取得する
		$cd["NAME"] = $line["summary"];
		// 金額を取得する
		$cd["TRNAMT"] = (string)((double)parse_amount($line["deposit"]) - (double)parse_amount($line["withdrawals"]));
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

?>
