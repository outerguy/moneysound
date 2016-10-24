<?php
/*
mhbk.inc: みずほ銀行のOFXよりOFXファイルを変換・結合する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["txbCustNo"];
$pass = $ofxforms["PASSWD_LoginPwdInput"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["txbTestWord"]), "Shift_JIS", "UTF-8"));
$sesscookie = $ofxforms["sesscookie"];
$accesskey = $ofxforms["accesskey"];

$resp = array();
$sid = 0;
$method = "";
$query = "";
$cookie = "";
$head = "";
$body = "";

$ofx = "";

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
	list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(strip_tags($a["innerHTML"]) == "みずほダイレクト") {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 安全にご利用いただくためのご注意画面が表示される場合、改めてログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "みずほダイレクトログイン画面に進む") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// リダイレクトする
	$metas = parse_tag($body, "meta");
	$c = parse_tag_search($metas, "http-equiv", "refresh");
	if($c != -1) {
		list($dummy, $uri) = explode("=", $metas[$c]["content"], 2);
		
		$method = "GET";
		$uris = parse_uri(trim($uri), $uris);
		$query = "";
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	}
	
	$loop = 0;
	do {
		// お客さま番号を入力する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "LOGBNK_00000B");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$queries["txbCustNo"] = "txbCustNo=" . $user;
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = mhbk_update_cookie($head, $cookie);
				list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
			}
		}
	} while($loop++ < ENV_NUM_HTTP11_RETRY && strpos($body, "お客さま番号を入力") !== false);
}

if(strpos($body, "合言葉確認１") !== false) {
	// 合言葉を入力する（１）
	$spans = parse_tag($body, "span");
	$c = parse_tag_search($spans, "id", "txtQuery");
	if($c != -1) {
		$shitsumon = trim(strip_tags($spans[$c]["innerHTML"]));
	}
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "LOGWRD_00100B");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) {
			if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			if($input["value"] == "次へ" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $input["onclick"], $matches) > 0) {
				$forms[$c]["action"] = $matches[1] . ".do";
			}
		}
		$queries["txbTestWord"] = "txbTestWord=";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mhbk_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("txbTestWord=", "txbTestWord=" . $auth, $query);
	list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "合言葉確認２") !== false) {
	// 合言葉を入力する（２）
	$spans = parse_tag($body, "span");
	$c = parse_tag_search($spans, "id", "txtQuery");
	if($c != -1) {
		$shitsumon = trim(strip_tags($spans[$c]["innerHTML"]));
	}
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "LOGWRD_00100B");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) {
			if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			if($input["value"] == "次へ" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $input["onclick"], $matches) > 0) {
				$forms[$c]["action"] = $matches[1] . ".do";
			}
		}
		$queries["txbTestWord"] = "txbTestWord=";
		if(isset($queries["chkConfItemChk"]) == true) {
			if(ENV_BOOL_ADD_RISKBASE == true) {
				// 次回ログイン時に合言葉を入力しない
				unset($queries["chkConfItemChk"]);
			} else {
				// 次回ログイン時に合言葉を入力する
				$queries["chkConfItemChk"] = "chkConfItemChk=on";
			}
		}
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mhbk_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 2;
	$head = "";
	$body = "";
} else if($sid == 2) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("txbTestWord=", "txbTestWord=" . $auth, $query);
	list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "ログインパスワードを半角英数字で入力") !== false) {
	// ログインパスワードを入力する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "LOGBNK_00005B");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["Cancel"]) == true) unset($queries["Cancel"]);
		$queries["PASSWD_LoginPwdInput"] = "PASSWD_LoginPwdInput=" . $pass;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "txbTestWord";
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
} else if(strpos($body, "サービスはご利用いただけません") !== false || strpos($body, "以下の時間帯は、インターネットバンキングをご利用いただけません") !== false) {
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
	// 入出金明細照会画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MENTOP_02000B");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$as = parse_tag($body, "a");
		foreach($as as $a) if(trim(strip_tags($a["innerHTML"])) == "入出金明細照会" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $a["onclick"], $matches) > 0) {
			$forms[$c]["action"] = $matches[1] . ".do";
			break;
		}
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
	
	$accounts = array();
	
	// 口座数を取得する
	$selects = parse_tag($body, "select");
	$c = parse_tag_search($selects, "name", "lstAccSel");
	if($c != -1) {
		$options = parse_tag($selects[$c]["innerHTML"], "option");
		$c = 0;
		foreach($options as $option) {
			list($branchname, $accttype, $acctid) = explode(" ", trim(strip_tags($option["innerHTML"])), 3);
			$accounts[$c]["id"] = $option["value"];
			$accounts[$c]["branchname"] = $branchname;
			$accounts[$c]["acctid"] = $acctid;
			$accounts[$c]["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $branchname, $accttype));
			$c++;
		}
	}
	
	// MSMoneyダウンロード画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "ACCHST_04000B");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$as = parse_tag($body, "a");
		foreach($as as $a) if(trim(strip_tags($a["innerHTML"])) == "こちら" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $a["href"], $matches) > 0) {
			$forms[$c]["action"] = $matches[1] . ".do";
			break;
		}
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
	
	// 口座数分ループする
	foreach($accounts as $account) {
		// 実行時間（タイムアウト）を再設定する
		@set_time_limit(ENV_NUM_TIMEOUT);
		
		// 明細ダウンロード口座・範囲画面を取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "ACCMNY_04610B");
		if($c != -1) {
			$queries = array();
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			foreach($inputs as $input) {
				if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
				if($input["value"] == "次へ" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $input["onclick"], $matches) > 0) {
					$forms[$c]["action"] = $matches[1] . ".do";
				}
			}
			$selects = parse_tag($forms[$c]["innerHTML"], "select");
			foreach($selects as $select) {
				$options = parse_tag($select["innerHTML"], "option");
				if(count($options) > 0) if($select["name"] != "") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($options[0]["value"]);
			}
			$queries["rdoAccSel"] = "rdoAccSel=" . $account["id"];
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = mhbk_update_cookie($head, $cookie);
				list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
			}
		}
		
		// リダイレクトする
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "ACCMNY_04620B");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = mhbk_update_cookie($head, $cookie);
				list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
			}
		}
		
		$body_old = $body;
		
		// OFXファイルを取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "ACCMNY_04630B");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			
			$as = parse_tag($body, "a");
			foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "width", "135") != -1 && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $a["href"], $matches) > 0) {
				$forms[$c]["action"] = $matches[1] . ".do";
				break;
			}
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
		
		// OFXファイルを変換する
		if(strpos($head, "Content-Type: text/html") === false) {
			$str = convert_ofx($body);
			
			// BANKTRANLISTを切り出す
			$banktranlists = parse_tag($str, "BANKTRANLIST");
			foreach($banktranlists as $banktranlist) {
				$buf = "\r\n";
				$buf .= parse_tag($banktranlist["innerHTML"], "DTSTART")[0]["match"] . "\r\n";
				$buf .= parse_tag($banktranlist["innerHTML"], "DTEND")[0]["match"] . "\r\n";
				
				$stmttrns = parse_tag($banktranlist["innerHTML"], "STMTTRN");
				
				// STMTTRNを昇順に挿入ソートする
				for($i = 1; $i < count($stmttrns); $i++) {
					$temp = $stmttrns[$i];
					$p = parse_tag($temp["innerHTML"], "DTPOSTED")[0]["innerHTML"];
					for($j = $i - 1; $j >= 0; $j--) {
						if(parse_tag($stmttrns[$j]["innerHTML"], "DTPOSTED")[0]["innerHTML"] <= $p) break;
						$stmttrns[$j + 1] = $stmttrns[$j];
					}
					$stmttrns[$j + 1] = $temp;
				}
				foreach($stmttrns as $stmttrn) $buf .= $stmttrn["match"] . "\r\n";
				
				// BANKTRANLISTをソート後の文字列に置換する
				$str = str_replace($banktranlist["innerHTML"], $buf, $str);
			}
			
			// BRANCHIDを補正する
			// $str = str_replace("<BRANCHID>0" . $account["branchid"] . "</BRANCHID>", "<BRANCHID>" . $account["branchid"] . "</BRANCHID>", $str);
			$str = preg_replace("/(<BRANCHID>)0([0-9]{3})(<\/BRANCHID>)/", "\\1\\2\\3", $str);
			
			// ACCTIDを補正する
			$str = str_replace("<ACCTID>00" . $account["acctid"] . "</ACCTID>", "<ACCTID>" . $account["acctid"] . "</ACCTID>", $str);
			
			// MEMOをNAMEに補正する
			$str = str_replace(array("<MEMO>", "</MEMO>"), array("<NAME>", "</NAME>\r\n<MEMO>" . ENV_STR_OFX_MEMO . "</MEMO>"), $str);
			
			// TRNAMTの数値の書式を補正する
			$trnamts = parse_tag($str, "TRNAMT");
			foreach($trnamts as $trnamt) $str = str_replace($trnamt["match"], "<TRNAMT>" . (string)((integer)$trnamt["innerHTML"]) . "</TRNAMT>", $str);
			
			// BALAMTの数値の書式を補正する
			$balamts = parse_tag($str, "BALAMT");
			foreach($balamts as $balamt) $str = str_replace($balamt["match"], "<BALAMT>" . (string)((integer)$balamt["innerHTML"]) . "</BALAMT>", $str);
			
			// FITIDの数値を補正する
			$fitids = parse_tag($str, "FITID");
			$fitidbuf = "";
			$cd_num = 0;
			foreach($fitids as $fitid) {
				if(substr($fitid["innerHTML"], 0, 8) == substr($fitidbuf, 0, 8)) $cd_num++; else $cd_num = 0;
				$str = str_replace($fitid["match"], "<FITID>" . substr($fitid["innerHTML"], 0, 8) . sprintf("%04d", (integer)$account["id"]) . "000" . sprintf("%05d", $cd_num) . "</FITID>", $str);
				$fitidbuf = $fitid["innerHTML"];
			}
			
			// 口座名称を出力する
			if($account["acctname"] != "") $str = str_replace("</STMTRS>", "<MKTGINFO>" . $account["acctname"] . "</MKTGINFO>\r\n</STMTRS>", $str);
			
			// OFXファイルが複数となる場合は連結する
			if($ofx != "") {
				$c = strpos($str, "<BANKMSGSRSV1>");
				if($c !== false) $str = substr($str, $c + 14);
				$c = strpos($ofx, "</BANKMSGSRSV1>");
				if($c !== false) $ofx = substr($ofx, 0, $c - 1) . $str;
			} else {
				$ofx .= $str;
			}
		}
		
		$body = $body_old;
		
		// 前画面を取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "ACCMNY_04630B");
		if($c != -1) {
			$queries = array();
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			foreach($inputs as $input) {
				if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
				if($input["value"] == "口座・範囲を再選択" && preg_match("/[^\']*\'\/([^\']*)\'[^\']*/", $input["onclick"], $matches) > 0) {
					$forms[$c]["action"] = $matches[1] . ".do";
				}
			}
			$selects = parse_tag($forms[$c]["innerHTML"], "select");
			foreach($selects as $select) {
				$options = parse_tag($select["innerHTML"], "option");
				if(count($options) > 0) if($select["name"] != "") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($options[0]["value"]);
			}
			$queries["rdoAccSel"] = "rdoAccSel=" . $account["id"];
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = mhbk_update_cookie($head, $cookie);
				list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
			}
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログアウト") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = mhbk_update_cookie($head, $cookie);
		list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
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
			$cookie = mhbk_update_cookie($head, $cookie);
			list($head, $body) = mhbk_http11($method, $uris, $query, $cookie);
		}
	}
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	if($ofx == "") {
		// OFXデータが存在しない場合
		$resp["ofx"] = generate_ofx($resp["status"]);
	} else {
		// OFXデータが存在する場合
		$resp["ofx"] = $ofx;
	}
}
return $resp;

// HTTP/1.1
function mhbk_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.mizuhobank\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function mhbk_update_cookie($head, $cookie) {
	return update_cookie(array("PMData", "BROWSER_TOKEN", "JSESSIONID", "Token", "ServerName"), parse_header($head, "set-cookie"), $cookie);
}

?>
