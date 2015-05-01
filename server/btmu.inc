<?php
/*
btmu.inc: 三菱東京UFJ銀行のCSVよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = str_replace("-", "", $ofxforms["KEIYAKU_NO"]);
$pass = $ofxforms["PASSWORD"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["PASSWORD2"]), "Shift_JIS", "UTF-8"));
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
	list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログイン") != -1 && preg_match("/javascript:[^\']+?\'([^\']+)\'.*?/i", $a["href"], $matches) > 0) {
		$method = "GET";
		$uris = parse_uri($matches[1], $uris);
		$query = "";
		$cookie = btmu_update_cookie($head, $cookie);
		list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=AA011_001"; // gotoPageFromAA011()
		$queries["KEIYAKU_NO"] = "KEIYAKU_NO=" . $user;
		$queries["PASSWORD"] = "PASSWORD=" . $pass;
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = btmu_update_cookie($head, $cookie);
		list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = btmu_update_cookie($head, $cookie);
			list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
		}
	}
}

if(strpos($body, "普段と異なる環境からのアクセス") !== false) {
	// ワンタイムパスワードを入力する
	$divs = parse_tag($body, "div");
	$c = parse_tag_search($divs, "class", "serviceContents");
	if($c != -1) {
		$ps = parse_tag($divs[$c]["innerHTML"], "p");
		$c = parse_tag_search($ps, "class", "section");
		if($c != -1) $shitsumon = trim(strip_tags($ps[$c]["innerHTML"]));
	}
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=AA012_001"; // gotoPageFromAA012()
		$queries["PASSWORD"] = "PASSWORD=";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = btmu_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("PASSWORD=", "PASSWORD=" . $auth, $query);
	list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = btmu_update_cookie($head, $cookie);
			list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
		}
	}
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "PASSWORD2";
	$resp["addition"] = $shitsumon;
	$resp["sid"] = $sid;
	$resp["sesscookie"] = sess_encode(implode("\t", array($cookie, ENV_STR_SESSION_PADDING)));
	$resp["accesskey"] = sess_encode(implode("\t", array((string)$sid, $method, $uris["scheme"] . "://" . $uris["host"] . $uris["path"], $query, $pass, ENV_STR_SESSION_PADDING)));
	
	$mfachallengetrnrs = "";
	$mfachallengetrnrs .= "<MFACHALLENGETRNRS>";
	$mfachallengetrnrs .= "<MFACHALLENGERS>";
	$mfachallengetrnrs .= "<MFACHALLENGE>";
	$mfachallengetrnrs .= "<MFAPHRASEID>" . $resp["aid"] . "</MFAPHRASEID>";
	$mfachallengetrnrs .= "<MFAPHRASELABEL>" . mb_convert_string($resp["addition"]) . "</MFAPHRASELABEL>";
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
	
	// 口座一覧（残高）をみる画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$as = parse_tag($body, "a");
		foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "口座一覧（残高）をみる") != -1 && preg_match("/javascript:[^\']+?\'([^\']+)\'.*?/i", $a["onclick"], $matches) > 0) {
			$queries["_TRANID"] = "_TRANID=" . $matches[1];
			break;
		}
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = btmu_update_cookie($head, $cookie);
		list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
	}
	
	$accs = array();
	
	// 普通口座の一覧を取得する
	$i = 0;
	$tables = parse_tag($body, "table");
	$c = parse_tag_search($tables, "summary", "普通預金一覧。左の列から、口座の種類、口座情報、明細/振込・振替へ、関連リンク。");
	if($c != -1) {
		$captions = parse_tag($tables[$c]["innerHTML"], "caption");
		if(count($captions) > 0) $accttype = trim(strip_tags(str_replace("預金一覧", "", $captions[0]["innerHTML"])));
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		foreach($trs as $tr) {
			$tds = parse_tag($tr["innerHTML"], "td");
			$d = parse_tag_search($tds, "class", "balance_info");
			if($d != -1) {
				// 残高を取得する
				$ps = parse_tag($tds[$d]["innerHTML"], "p");
				$e = parse_tag_search($ps, "class", "unit");
				if($e != -1) $accounts[$i]["balance"] = parse_amount(strip_tags($ps[$e]["innerHTML"]));
				
				// 口座種目を取得する（「普通」固定）
				$accounts[$i]["accttype"] = btmu_get_accttype($accttype);
				
				// 支店名・口座番号を取得する
				$spans = parse_tag($tds[$d]["innerHTML"], "span");
				$e = parse_tag_search($spans, "class", "unit");
				if($e != -1) {
					$p = strrpos($spans[$e]["innerHTML"], " ");
					if($p !== false) {
						$accounts[$i]["branchid"] = btmu_get_branchid($settings, substr($spans[$e]["innerHTML"], 0, $p));
						$accounts[$i]["acctid"] = trim(strip_tags(substr($spans[$e]["innerHTML"], $p)));
						$accounts[$i]["acctname"] = implode(mb_convert_string(ENV_CHR_CONCATENATOR), array($settings["name"], mb_convert_string(substr($spans[$e]["innerHTML"], 0, $p)), mb_convert_string($accttype)));
					}
				
				}
				
				// 明細番号を取得する
				$as = parse_tag($tr["innerHTML"], "a");
				foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "明細") != -1 && preg_match("/javascript:[^\(]+?\((\d+)\).*?/i", $a["onclick"], $matches) > 0) {
					$accounts[$i]["id"] = trim($matches[1]);
					$i++;
					break;
				}
			}
		}
	}
	
	// 口座数分ループする
	for($i = 0; $i < count($accounts); $i++) {
		// 実行時間（タイムアウト）を再設定する
		@set_time_limit(ENV_NUM_TIMEOUT);
		
		// 明細画面を取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "MainForm");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$queries["SENTAKU"] = "SENTAKU=" . $accounts[$i]["id"];
			$queries["_TRANID"] = "_TRANID=CO101_003"; // 明細
			$queries["_TARGET"] = "_TARGET=" . $forms[$c]["name"];
			$queries["_FRAMID"] = "_FRAMID=";
			$queries["_TARGETWINID"] = "_TARGETWINID=";
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = btmu_update_cookie($head, $cookie);
			list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
		}
		
		if(strpos($body, "照会可能な明細はありません") === false) {
			// 明細をダウンロード画面を取得する
			$forms = parse_tag($body, "form");
			$c = parse_tag_search($forms, "name", "MainForm");
			if($c != -1) {
				$inputs = parse_tag($forms[$c]["innerHTML"], "input");
				$queries = array();
				foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
				$selects = parse_tag($forms[$c]["innerHTML"], "select");
				foreach($selects as $select) {
					$options = parse_tag($select["innerHTML"], "option");
					foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
				}
				$queries["_TRANID"] = "_TRANID=CP105_006"; // 明細をダウンロード
				$queries["_TARGET"] = "_TARGET=" . $forms[$c]["name"];
				$queries["_FRAMID"] = "_FRAMID=";
				$queries["_TARGETWINID"] = "_TARGETWINID=";
				
				$method = $forms[$c]["method"];
				$uris = parse_uri($forms[$c]["action"], $uris);
				$query = implode("&", $queries);
				$cookie = btmu_update_cookie($head, $cookie);
				list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
			}
			
			$body_old = $body;
			
			// ダウンロード画面を取得する
			$forms = parse_tag($body, "form");
			$c = parse_tag_search($forms, "name", "MainForm");
			if($c != -1) {
				$inputs = parse_tag($forms[$c]["innerHTML"], "input");
				$queries = array();
				foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
				$selects = parse_tag($forms[$c]["innerHTML"], "select");
				foreach($selects as $select) {
					$options = parse_tag($select["innerHTML"], "option");
					foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
				}
				$queries["_TRANID"] = "_TRANID=CP107_001"; // ダウンロード（CSV形式）
				$queries["_TARGET"] = "_TARGET=" . $forms[$c]["name"];
				$queries["_FRAMID"] = "_FRAMID=";
				$queries["_TARGETWINID"] = "_TARGETWINID=";
				
				$method = $forms[$c]["method"];
				$uris = parse_uri($forms[$c]["action"], $uris);
				$query = implode("&", $queries);
				$cookie = btmu_update_cookie($head, $cookie);
				list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
			}
			
			// CSVファイルを取得する
			$forms = parse_tag($body, "form");
			$c = parse_tag_search($forms, "name", "MainForm");
			if($c != -1) {
				$inputs = parse_tag($forms[$c]["innerHTML"], "input");
				$queries = array();
				foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
				$selects = parse_tag($forms[$c]["innerHTML"], "select");
				foreach($selects as $select) {
					$options = parse_tag($select["innerHTML"], "option");
					foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
				}
				$queries["_TRANID"] = "_TRANID=CP107_003"; // ダウンロード（CSV形式）
				$queries["_TARGET"] = "_TARGET=" . $forms[$c]["name"];
				$queries["_FRAMID"] = "_FRAMID=";
				$queries["_TARGETWINID"] = "_TARGETWINID=";
				
				$method = $forms[$c]["method"];
				$uris = parse_uri($forms[$c]["action"], $uris);
				$query = implode("&", $queries);
				$cookie = btmu_update_cookie($head, $cookie);
				list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
				if(strpos($head, "Content-Type: text/html") === false) {
					$details = array();
					$j = 0;
					$lines = parse_csv($body);
					foreach($lines as $line) {
						$c = count($line); // 6=Eco明細でない 9=Eco明細
						if(($c == 6 || $c == 9) && $line[0] != "日付") {
							$details[$j]["date"] = parse_date($line[0]);
							$details[$j]["summary"] = $line[1] . ($line[2] != ""? ENV_CHR_CONCATENATOR . $line[2]: "");
							$details[$j]["withdrawals"] = parse_amount($line[3]);
							$details[$j]["deposit"] = parse_amount($line[4]);
							$details[$j]["memo"] = ($line[6] != ""? $line[6]: ENV_STR_OFX_MEMO);
							// $details[$j]["trntype"] = ($line[8] != ""? $line[8]: "");
							$j++;
						}
					}
					$accounts[$i]["details"] = $details;
				}
			}
			
			$body = $body_old;
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=AD001_022"; // ログアウト
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = btmu_update_cookie($head, $cookie);
		list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = btmu_update_cookie($head, $cookie);
			list($head, $body) = btmu_http11($method, $uris, $query, $cookie);
		}
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
		$bankmsgsrsv1 .= btmu_parse_details($account);
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
function btmu_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.bk\.mufg\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function btmu_update_cookie($head, $cookie) {
	return update_cookie(array("IW_INFO", "JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function mapion_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.mapion\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

// 支店名より店番を検索する
function btmu_get_branchid($settings, $str) {
	$ret = mb_convert_string($str);
	
	// 検索画面を取得する
	$method = "GET";
	$uris = parse_uri($settings["getbranchid"] . urlencode($ret));
	$query = "";
	$cookie = "";
	list($head, $body) = mapion_http11($method, $uris, $query, $cookie);
	
	$ths = parse_tag($body, "th");
	foreach($ths as $th) {
		list($k, $v) = explode(":", trim(strip_tags($th["innerHTML"])));
		$k = trim($k);
		$v = trim($v);
		if($k == "店番") {
			$ret = $v;
			break;
		}
	}
	
	return $ret;
}

// 預金種類よりACCTTYPEを決定する
function btmu_get_accttype($str) {
	$ret = ENV_STR_ACCTTYPE_SAVINGS;
	switch($str) {
	case "普通":
	case "定期":
		$ret = ENV_STR_ACCTTYPE_SAVINGS;
		break;
	default:
		$ret = ENV_STR_ACCTTYPE_CHECKING;
		break;
	}
	return $ret;
}

function btmu_parse_details($account) {
	$ret = "";
	$cds = array();
	$cd_date = "";
	$cd_num = 0;
	if(is_array($account["details"]) == true) foreach($account["details"] as $line) {
		$cd = array();
		if(mb_strpos($line["summary"], "振込") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
		} else if(mb_strpos($line["summary"], "振替") === 0) { // 未確認
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_XFER;
		} else if(mb_strpos($line["summary"], "通帳") === 0) { // 未確認
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
		} else if(mb_strpos($line["summary"], "カード") === 0) {
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
		} else if(mb_strpos($line["summary"], "利息") === 0) {
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
		$cd["NAME"] = mb_convert_string($line["summary"]);
		
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
