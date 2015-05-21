<?php
/*
jist.inc: 日本インベスター・ソリューション・アンド・テクノロジーのHTMLよりOFXファイルを生成する
Copyright (C) 2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["ACCOUNT"];
$pass = $ofxforms["PASS"];

$resp = array();

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = jist_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "確定拠出年金インターネットサービス") != -1) {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = jist_update_cookie($head, $cookie);
	list($head, $body) = jist_http11($method, $uris, $query, $cookie);
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
	$cookie = jist_update_cookie($head, $cookie);
	list($head, $body) = jist_http11($method, $uris, $query, $cookie);
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "FORM_301_01");
if(count($forms) >= 1) {
	$inputs = parse_tag($forms[0]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["ACCOUNT"] = "ACCOUNT=" . $user;
	$queries["PASS"] = "PASS=" . $pass;
	
	$method = $forms[0]["method"];
	$uris = parse_uri($forms[0]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = jist_update_cookie($head, $cookie);
	list($head, $body) = jist_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
	}
}

// リダイレクトする
$forms = parse_tag($body, "form");
if(count($forms) == 1) {
	$inputs = parse_tag($forms[0]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	
	$method = $forms[0]["method"];
	$uris = parse_uri($forms[0]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = jist_update_cookie($head, $cookie);
	list($head, $body) = jist_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "システムメンテナンス中") !== false) { // 要検討
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回アクセス") === false) {
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
	$marginbalance = "0";
	$availcash = "0";
	$exchanges = array();
	$securities = array();
	$banks = array();
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	$acctid = "";
	$acctname = "";
	
	// 残高・時価評価額照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_MENU") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_ActionID"] = "_ActionID=goDCCGW301"; // 残高・時価評価額照会
		$queries["EVENT"] = "EVENT=DCWGA201-02";
		$queries["MENUNO"] = "MENUNO=01";
		$queries["MAINMENU"] = "MAINMENU=01";
		$queries["LEFTMENU"] = "LEFTMENU=12";
		
		$head .= "Set-Cookie: DMY=0\r\n";
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$bufs = array();
	
	$trs = parse_tag($body, "tr", true);
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 4 && $tds[2]["colspan"] == "4") {
			// 商品番号を取得する
			$bufs["UNIQUEID"] = trim(strip_tags($tds[0]["innerHTML"]));
			$bufs["UNIQUEIDTYPE"] = $settings["code"];
			
			// 商品名を取得する
			$bufs["SECNAME"] = trim(strip_tags($tds[2]["innerHTML"]));
			
			$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
			$bufs["HELDINACCT"] = ENV_STR_OFX_TRNTYPE_CASH; // 現金
			$bufs["POSTYPE"] = "LONG";
		}
		if(count($tds) == 5 && $tds[0]["width"] != "") {
			// 残高を取得する
			$bufs["UNITS"] = parse_amount(strip_tags($tds[1]["innerHTML"]));
			
			// 時価単価を取得する
			$brs = explode("<BR>", $tds[2]["innerHTML"]);
			$bufs["UNITPRICE"] = parse_amount(trim(strip_tags($brs[0])));
			if($bufs["UNITPRICE"] == "0") $bufs["UNITPRICE"] = "1";
			
			// 時価単価の単位を取得する
			$bufs["UNITRATE"] = parse_amount(trim(strip_tags($brs[1])));
			if($bufs["UNITRATE"] == "0") $bufs["UNITRATE"] = "1";
			
			// 時価評価額を補正する
			$bufs["UNITPRICE"] = (string)((double)$bufs["UNITPRICE"] / (double)$bufs["UNITRATE"]);
			
			// 数量を取得する
			$bufs["MKTVAL"] = parse_amount(strip_tags($tds[3]["innerHTML"]));
			
			// 時価基準日を取得する
			$bufs["DTPRICEASOF"] = ($tds[4]["innerHTML"] == "－"? ENV_STR_DATE_TODAY: parse_date($tds[4]["innerHTML"]));
			
			// 損益を算出することができない仕様のため、デフォルトの固定文字列とする
			$bufs["MEMO"] = ENV_STR_OFX_MEMO;
			
			array_push($securities, $bufs);
			$bufs = array();
		}
	}
	
	// 待機資金を取得する
	$tds = parse_tag($body, "td", true);
	foreach($tds as $td) {
		if($td["rowspan"] == "2" && $td["align"] == "right") {
			$marginbalance = parse_amount(strip_tags($td["innerHTML"]));
			$availcash = $marginbalance;
			break;
		}
	}
	
	// 取引履歴照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_MENU") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_ControlID"] = "_ControlID=DCACW000Control";
		$queries["_PageID"] = "_PageID=DCAGW308";
		$queries["_ActionID"] = "_ActionID=goDCDGW301"; // 取引履歴照会
		$queries["EVENT"] = "EVENT=DCWGA201-03";
		$queries["MENUNO"] = "MENUNO=01";
		$queries["MAINMENU"] = "MAINMENU=01";
		$queries["LEFTMENU"] = "LEFTMENU=13";
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 期間、および表示順を指定し、画面を再取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_301_01") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["FYM"] = "FYM=" . substr(ENV_STR_DATE_PASTDAY, 0, 6);
		$queries["TYM"] = "TYM=" . substr(ENV_STR_DATE_TODAY, 0, 6);
		$queries["SORT"] = "SORT=1"; // 古い順から表示
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 取引履歴を取得する
	$tables = parse_tag($body, "table", true);
	$c = parse_tag_search($tables, "border", "1");
	if($c != -1) {
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		for($i = 0; $i < 2; $i++) array_shift($trs); // ヘッダーを読み飛ばす
		while(count($trs) > 0) {
			$bufs = array();
			
			// 2行ずつ取り出す
			$tr_left = array_shift($trs);
			$tr_right = array_shift($trs);
			
			// 表の左側を処理する
			$tds = parse_tag($tr_left["innerHTML"], "td");
			
			// 発注日・約定日を取得する
			$nobrs = parse_tag($tds[0]["innerHTML"], "nobr");
			if(count($nobrs) == 3) {
				$bufs["DTPOSTED"] = parse_date($nobrs[0]["innerHTML"]); // 発注日
				$bufs["DTTRADE"] = parse_date($nobrs[1]["innerHTML"]); // 約定日
			}
			
			// 売買区分・取引状況を取得する
			$brs = explode("<BR>", $tds[1]["innerHTML"]);
			if(count($brs) == 3) {
				$bufs["NAME"] = trim(strip_tags($brs[0])); // 取引区分
				$bufs["MEMO"] = $brs[1]; // 売買区分
			}
			
			// 商品番号を取得する
			$bufs["UNIQUEID"] = strip_tags($tds[2]["innerHTML"]);
			$bufs["UNIQUEIDTYPE"] = $settings["code"];
			
			// 商品名を取得する
			$bufs["SECNAME"] = strip_tags($tds[3]["innerHTML"]);
			$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
			
			// 表の右側を処理する
			$tds = parse_tag($tr_right["innerHTML"], "td");
			
			// 約定数量を取得する
			$bufs["UNITS"] = parse_amount(strip_tags($tds[0]["innerHTML"]));
			
			// 約定単価を取得する
			$bufs["UNITPRICE"] = parse_amount(strip_tags($tds[1]["innerHTML"]));
			if($bufs["UNITPRICE"] == "0") $bufs["UNITPRICE"] = "1";
			
			// 約定単価の単位を取得する
			$bufs["UNITRATE"] = parse_amount(strip_tags($tds[2]["innerHTML"]));
			if($bufs["UNITRATE"] == "0") $bufs["UNITRATE"] = "1";
			
			// 約定単価を補正する
			$bufs["UNITPRICE"] = (string)((integer)$bufs["UNITPRICE"] / (integer)$bufs["UNITRATE"]);
			
			// 精算金額を取得する
			$bufs["TOTAL"] = parse_amount(strip_tags($tds[3]["innerHTML"]));
			if($bufs["TOTAL"] == "0") $bufs["TOTAL"] = (string)((integer)$bufs["UNITS"] * (double)$bufs["UNITPRICE"]);
			
			$bufs["SUBACCTSEC"] = ENV_STR_OFX_CASH; // 現金
			$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
			
			// 定時拠出の場合、INVTRANLIST（現金）の明細を生成する
			if($bufs["NAME"] == "定時拠出") {
				$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
				$bufs["TRNAMT"] = $bufs["TOTAL"];
				$bufs["MEMO"] = ENV_STR_OFX_MEMO;
				
				array_push($banks, $bufs);
			}
			
			// 未約定の取引を読み飛ばす
			if($brs[2] == "処理中") continue;
			
			switch($bufs["MEMO"]) {
			case "再投資":
				$bufs["BUYTYPE"] = ENV_STR_OFX_REINVEST; // 再投資
				$bufs["TOTAL"] = "-" . $bufs["TOTAL"];
				break;
			case "売却":
			case "出金":
			case "払戻":
			case "返戻":
			case "解約":
				$bufs["BUYTYPE"] = ENV_STR_OFX_SELL; // 売付
				$bufs["UNITS"] = "-" . $bufs["UNITS"];
				break;
			case "購入":
			case "入金":
			case "払込":
			default:
				$bufs["BUYTYPE"] = ENV_STR_OFX_BUY; // 買付
				$bufs["TOTAL"] = "-" . $bufs["TOTAL"];
				break;
			}
			
			array_push($exchanges, $bufs);
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 個人属性画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_MENU") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_ControlID"] = "_ControlID=DCACW000Control";
		$queries["_PageID"] = "_PageID=DCAGW312";
		$queries["_ActionID"] = "_ActionID=doMenu_310"; // 個人属性
		$queries["EVENT"] = "EVENT=DCWGA101-03";
		$queries["MENUNO"] = "MENUNO=03";
		$queries["MAINMENU"] = "MAINMENU=03";
		$queries["LEFTMENU"] = "LEFTMENU=33";
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 個人登録情報照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_MENU") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_ControlID"] = "_ControlID=DCACW000Control";
		$queries["_PageID"] = "_PageID=DCAGW310";
		$queries["_ActionID"] = "_ActionID=goDCLGW311"; // 個人登録情報照会
		$queries["EVENT"] = "EVENT=DCWGA203-01";
		$queries["MENUNO"] = "MENUNO=03";
		$queries["MAINMENU"] = "MAINMENU=03";
		$queries["LEFTMENU"] = "LEFTMENU=31";
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 契約名、および加入者口座番号を取得する
	$trs = parse_tag($body, "tr", true);
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds == 2)) switch($tds[0]["innerHTML"]) {
		case "契約名":
			$acctname = ENV_CHR_CONCATENATOR . $tds[1]["innerHTML"];
			break;
		case "加入者口座番号":
			$acctid = $tds[1]["innerHTML"];
			break;
		default:
			break;
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "FORM_MENU") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_ControlID"] = "_ControlID=DCACW000Control";
		$queries["_DataStoreID"] = "_DataStoreID=DSDCACW000Control";
		$queries["_PageID"] = "_PageID=DCAGW312";
		$queries["_ActionID"] = "_ActionID=doLogout_Link"; // ログアウト
		$queries["EVENT"] = "EVENT=DCWGA101-05";
		$queries["MENUNO"] = "MENUNO=05";
		
		$head .= "Set-Cookie: NAMELO=1\r\n";
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jist_update_cookie($head, $cookie);
		list($head, $body) = jist_http11($method, $uris, $query, $cookie);
		break;
	}
	
	if($acctid == "") $acctid = $user;
	
	// INVTRANLIST（投資）
	$invtranlist = "";
	$invtranlist .= "<DTSTART>" . ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ . "</DTSTART>";
	$invtranlist .= "<DTEND>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTEND>";
	$invtranlist .= "\r\n";
	
	$cd_date = "";
	$cd_num = 0;
	
	foreach($exchanges as $exchange) {
		if($exchange["DTTRADE"] == $cd_date) $cd_num++; else $cd_num = 0;
		$fitid = $exchange["DTTRADE"] . "0000001" . sprintf("%05d", $cd_num);
		
		if($exchange["BUYTYPE"] == ENV_STR_OFX_REINVEST) {
			// 再投資の場合、再投資の明細を生成する
			$invtranlist .= "<" . $exchange["BUYTYPE"] . ">";
			$invtranlist .= "<INVTRAN>";
			$invtranlist .= "<FITID>" . $fitid . "</FITID>";
			$invtranlist .= "<DTTRADE>" . $exchange["DTTRADE"] . ENV_STR_OFX_TZ . "</DTTRADE>";
			$invtranlist .= "</INVTRAN>";
			$invtranlist .= "<SECID><UNIQUEID>" . $exchange["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $exchange["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$invtranlist .= "<INCOMETYPE>" . ENV_STR_OFX_TRNTYPE_DIV . "</INCOMETYPE>";
			$invtranlist .= "<TOTAL>" . $exchange["TOTAL"] . "</TOTAL>";
			$invtranlist .= "<SUBACCTSEC>" . $exchange["SUBACCTSEC"] . "</SUBACCTSEC>";
			$invtranlist .= "<UNITS>" . $exchange["UNITS"] . "</UNITS>";
			$invtranlist .= "<UNITPRICE>" . $exchange["UNITPRICE"] . "</UNITPRICE>";
			$invtranlist .= "</" . $exchange["BUYTYPE"] . ">";
			$invtranlist .= "\r\n";
		} else {
			// その他の場合、売買の明細を生成する
			$invtranlist .= "<" . $exchange["BUYTYPE"] . $exchange["CATEGORY"] . ">";
			$invtranlist .= "<INV" . $exchange["BUYTYPE"] . ">";
			$invtranlist .= "<INVTRAN>";
			$invtranlist .= "<FITID>" . $fitid . "</FITID>";
			$invtranlist .= "<DTTRADE>" . $exchange["DTTRADE"] . ENV_STR_OFX_TZ . "</DTTRADE>";
			$invtranlist .= "</INVTRAN>";
			$invtranlist .= "<SECID><UNIQUEID>" . $exchange["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $exchange["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$invtranlist .= "<UNITS>" . $exchange["UNITS"] . "</UNITS>";
			$invtranlist .= "<UNITPRICE>" . $exchange["UNITPRICE"] . "</UNITPRICE>";
			$invtranlist .= "<TOTAL>" . $exchange["TOTAL"] . "</TOTAL>";
			$invtranlist .= "<SUBACCTSEC>" . $exchange["SUBACCTSEC"] . "</SUBACCTSEC>";
			$invtranlist .= "<SUBACCTFUND>" . $exchange["SUBACCTFUND"] . "</SUBACCTFUND>";
			$invtranlist .= "</INV" . $exchange["BUYTYPE"] . ">";
			$invtranlist .= "<" . $exchange["BUYTYPE"] . "TYPE>" . $exchange["BUYTYPE"] . "</" . $exchange["BUYTYPE"] . "TYPE>";
			$invtranlist .= "</" . $exchange["BUYTYPE"] . $exchange["CATEGORY"] . ">";
			$invtranlist .= "\r\n";
		}
		
		$cd_date = $exchange["DTTRADE"];
	}
	
	$cd_date = "";
	$cd_num = 0;
	
	// INVTRANLIST（現金）
	foreach($banks as $bank) {
		if($bank["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
		$fitid = $bank["DTPOSTED"] . "0000000" . sprintf("%05d", $cd_num);
		
		$invtranlist .= "<INVBANKTRAN>";
		$invtranlist .= "<STMTTRN>";
		$invtranlist .= "<TRNTYPE>" . $bank["TRNTYPE"] . "</TRNTYPE>";
		$invtranlist .= "<DTPOSTED>" . $bank["DTPOSTED"] . ENV_STR_OFX_TZ . "</DTPOSTED>";
		$invtranlist .= "<TRNAMT>" . $bank["TRNAMT"] . "</TRNAMT>";
		$invtranlist .= "<FITID>" . $fitid . "</FITID>";
		$invtranlist .= "<NAME>" . $bank["NAME"] . "</NAME>";
		$invtranlist .= "<MEMO>" . $bank["MEMO"] . "</MEMO>";
		$invtranlist .= "</STMTTRN>";
		$invtranlist .= "<SUBACCTFUND>" . $bank["SUBACCTFUND"] . "</SUBACCTFUND>";
		$invtranlist .= "</INVBANKTRAN>";
		$invtranlist .= "\r\n";
		
		$cd_date = $bank["DTPOSTED"];
	}
	
	$invposlist = "";
	$seclist = "";
	
	// INVPOSLIST
	foreach($securities as $security) {
		// SECLIST
		$seclist .= "<" . $security["CATEGORY"] . "INFO><SECINFO>";
		$seclist .= "<SECID><UNIQUEID>" . $security["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$seclist .= "<SECNAME>" . $security["SECNAME"] . "</SECNAME>";
		$seclist .= "</SECINFO></" . $security["CATEGORY"] . "INFO>";
		$seclist .= "\r\n";
		
		$invposlist .= "<POS" . $security["CATEGORY"] . "><INVPOS>";
		$invposlist .= "<SECID><UNIQUEID>" . $security["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$invposlist .= "<HELDINACCT>" . $security["HELDINACCT"] . "</HELDINACCT>";
		$invposlist .= "<POSTYPE>" . $security["POSTYPE"] . "</POSTYPE>";
		$invposlist .= "<UNITS>" . $security["UNITS"] . "</UNITS>";
		$invposlist .= "<UNITPRICE>" . $security["UNITPRICE"] . "</UNITPRICE>";
		$invposlist .= "<MKTVAL>" . $security["MKTVAL"] . "</MKTVAL>";
		$invposlist .= "<DTPRICEASOF>" . $security["DTPRICEASOF"] . ENV_STR_OFX_TZ . "</DTPRICEASOF>";
		$invposlist .= "<MEMO>" . $security["MEMO"] . "</MEMO>";
		$invposlist .= "</INVPOS></POS" . $security["CATEGORY"] . ">";
		$invposlist .= "\r\n";
	}
	
	foreach($exchanges as $exchange) {
		if(strpos($seclist, $exchange["SECNAME"]) === false) {
			// SECLIST
			$seclist .= "<!-- SOLD -->";
			$seclist .= "<" . $exchange["CATEGORY"] . "INFO><SECINFO>";
			$seclist .= "<SECID><UNIQUEID>" . $exchange["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $exchange["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$seclist .= "<SECNAME>" . $exchange["SECNAME"] . "</SECNAME>";
			$seclist .= "</SECINFO></" . $exchange["CATEGORY"] . "INFO>";
			$seclist .= "\r\n";
		}
	}
	
	$invstmtmsgsrsv1 = "";
	$invstmtmsgsrsv1 .= "<INVSTMTMSGSRSV1>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVSTMTTRNRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<TRNUID>0</TRNUID>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<STATUS><CODE>0</CODE><SEVERITY>INFO</SEVERITY></STATUS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVSTMTRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<DTASOF>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTASOF>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<CURDEF>" . ENV_STR_OFX_CURRENCY_JPY . "</CURDEF>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVACCTFROM>";
	$invstmtmsgsrsv1 .= "<BROKERID>" . $settings["code"] . "</BROKERID>";
	$invstmtmsgsrsv1 .= "<ACCTID>" . $acctid . "</ACCTID>";
	$invstmtmsgsrsv1 .= "</INVACCTFROM>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVTRANLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= $invtranlist;
	$invstmtmsgsrsv1 .= "</INVTRANLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVPOSLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= $invposlist;
	$invstmtmsgsrsv1 .= "</INVPOSLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<INVBAL>";
	$invstmtmsgsrsv1 .= "<AVAILCASH>" . $availcash . "</AVAILCASH>";
	$invstmtmsgsrsv1 .= "<MARGINBALANCE>" . $marginbalance . "</MARGINBALANCE>";
	$invstmtmsgsrsv1 .= "<SHORTBALANCE>0</SHORTBALANCE>";
	$invstmtmsgsrsv1 .= "</INVBAL>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<MKTGINFO>" . $settings["name"] . $acctname . "</MKTGINFO>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "</INVSTMTRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "</INVSTMTTRNRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "</INVSTMTMSGSRSV1>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<SECLISTMSGSRSV1>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<SECLISTTRNRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<TRNUID>0</TRNUID>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<STATUS><CODE>0</CODE><SEVERITY>INFO</SEVERITY></STATUS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "</SECLISTTRNRS>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<SECLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= $seclist;
	$invstmtmsgsrsv1 .= "</SECLIST>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "</SECLISTMSGSRSV1>";
	$invstmtmsgsrsv1 .= "\r\n";
	
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	$resp["ofx"] = generate_ofx($resp["status"], $invstmtmsgsrsv1);
}
return $resp;

function jist_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.jis\-t\.(?:co|ne)\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function jist_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID", "BIGipServerpool_pcweb", "TSda4a5e", "DMY", "NAME", "NAMELO"), parse_header($head, "set-cookie"), $cookie);
}

?>
