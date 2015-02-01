<?php
/*
netbk.inc: 住信SBIネット銀行のCSVよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["userName"];
$pass = $ofxforms["loginPwdSet"];

$resp = array();

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = netbk_http11($method, $uris, $query, $cookie);

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
	}
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "LoginForm");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["userName"] = "userName=" . $user;
	$queries["loginPwdSet"] = "loginPwdSet=" . $pass;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = netbk_update_cookie($head, $cookie);
	list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "銀行からの重要なお知らせ") !== false) {
	// 重要なお知らせ画面の場合、スキップする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "form0103_01_100");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if(isset($queries["imptNtcCheck"]) == true) unset($queries["imptNtcCheck"]); // 「確認しました」チェックボックスをオフにする
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "form0103_01_105");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
	}
}

if(strpos($body, "現在この取引はお取扱いできません") !== false) {
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
	// 残高照会（口座別）画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "口座情報") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// 口座情報を取得する
	if(preg_match("/([0-9]{3})\-([0-9]{7})/", $body, $matches) > 0) {
		$branchid = $matches[1];
		$acctid = $matches[2];
	}
	
	$accounts = array();
	
	// 口座数を取得する
	$trs = parse_tag($body, "tr", true); // 再帰的に取得する
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 5) {
			$as = parse_tag($tds[4]["innerHTML"], "a");
			foreach($as as $a) if(trim(strip_tags($a["innerHTML"])) == "明細" && preg_match("/.*?\/([0-9]{2})\/(01|21)\/001\/01/", $a["href"], $matches) > 0) {
				$accounts[$c]["acctname"] = str_replace(array("口座", "&nbsp;-&nbsp;"), array("", ENV_CHR_CONCATENATOR), trim(strip_tags($tds[0]["innerHTML"])));
				$accounts[$c]["id"] = ($matches[2] == "01"? $matches[1]: "00"); // SBIハイブリッド預金を00とみなす
				$accounts[$c]["acctid"] = $matches[1]; // 01=代表口座 02-99=目的別口座
				$accounts[$c]["accttype"] = $matches[2]; // 01=円普通 21=SBIハイブリッド
				$accounts[$c]["page"] = $a["href"];
				$c++;
			}
		}
	}
	
	// お客さま情報照会・変更画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "お客さま情報照会・変更");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
	}
	
	// 支店名を取得する
	$otables = parse_tag($body, "table");
	foreach($otables as $otable) {
		$tables = parse_tag($otable["innerHTML"], "table");
		foreach($tables as $table) {
			$c = 0;
			$trs = parse_tag($table["innerHTML"], "tr");
			foreach($trs as $tr) {
				$divs = parse_tag($tr["innerHTML"], "div");
				if(count($divs) == 2 && trim(strip_tags($divs[0]["innerHTML"])) == "口座番号") {
					$bufs = explode(" ", preg_replace("/\s{2,}/", " ", str_replace("&nbsp;", "", trim(strip_tags($divs[1]["innerHTML"])))));
					for($i = 0; $i < count($accounts); $i++) $accounts[$i]["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $bufs[1], $accounts[$i]["acctname"]));
				}
			}
		}
	}
	
	$bankmsgsrsv1 = "";
	$bankmsgsrsv1 .= "<BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// 口座数分ループする
	foreach($accounts as $account) {
		// 実行時間（タイムアウト）を再設定する
		@set_time_limit(ENV_NUM_TIMEOUT);
		
		// 口座情報画面を取得する
		$method = "GET";
		$uris = parse_uri($account["page"], $uris);
		$query = "";
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
		
		// 入出金明細画面を取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "form0202_01_100");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$selects = parse_tag($forms[$c]["innerHTML"], "select");
			foreach($selects as $select) {
				$options = parse_tag($select["innerHTML"], "option");
				foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
			}
			$queries["termLink"] = "termLink=05"; // 期間指定
			$queries["term"] = "term=05"; // 期間指定
			$queries["acctBusPdCodeInput"] = "acctBusPdCodeInput=" . $account["acctid"] . $account["accttype"] . "001";
			$queries["dsplyTrmSpcfdYearFrom"] = "dsplyTrmSpcfdYearFrom=" . substr(ENV_STR_DATE_PASTDAY, 0, 4);
			$queries["dsplyTrmSpcfdMonthFrom"] = "dsplyTrmSpcfdMonthFrom=" . substr(ENV_STR_DATE_PASTDAY, 4, 2);
			$queries["dsplyTrmSpcfdDayFrom"] = "dsplyTrmSpcfdDayFrom=" . substr(ENV_STR_DATE_PASTDAY, 6, 2);
			$queries["dsplyTrmSpcfdYearTo"] = "dsplyTrmSpcfdYearTo=" . substr(ENV_STR_DATE_TODAY, 0, 4);
			$queries["dsplyTrmSpcfdMonthTo"] = "dsplyTrmSpcfdMonthTo=" . substr(ENV_STR_DATE_TODAY, 4, 2);
			$queries["dsplyTrmSpcfdDayTo"] = "dsplyTrmSpcfdDayTo=" . substr(ENV_STR_DATE_TODAY, 6, 2);
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = netbk_update_cookie($head, $cookie);
			list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
		}
		
		$body_old = $body;
		
		// CSVファイルをダウンロードする
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "form0202_01_100");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$selects = parse_tag($forms[$c]["innerHTML"], "select");
			foreach($selects as $select) {
				$options = parse_tag($select["innerHTML"], "option");
				foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
			}
			$queries["termLink"] = "termLink=05"; // 期間指定
			$queries["term"] = "term=05"; // 期間指定
			$queries["acctBusPdCodeInput"] = "acctBusPdCodeInput=" . $account["acctid"] . $account["accttype"] . "001";
			$queries["dsplyTrmSpcfdYearFrom"] = "dsplyTrmSpcfdYearFrom=" . substr(ENV_STR_DATE_PASTDAY, 0, 4);
			$queries["dsplyTrmSpcfdMonthFrom"] = "dsplyTrmSpcfdMonthFrom=" . substr(ENV_STR_DATE_PASTDAY, 4, 2);
			$queries["dsplyTrmSpcfdDayFrom"] = "dsplyTrmSpcfdDayFrom=" . substr(ENV_STR_DATE_PASTDAY, 6, 2);
			$queries["dsplyTrmSpcfdYearTo"] = "dsplyTrmSpcfdYearTo=" . substr(ENV_STR_DATE_TODAY, 0, 4);
			$queries["dsplyTrmSpcfdMonthTo"] = "dsplyTrmSpcfdMonthTo=" . substr(ENV_STR_DATE_TODAY, 4, 2);
			$queries["dsplyTrmSpcfdDayTo"] = "dsplyTrmSpcfdDayTo=" . substr(ENV_STR_DATE_TODAY, 6, 2);
			$queries["_ActionID"] = "_ActionID=doCSVDownload"; // CSVダウンロード
			if(isset($queries["ACT_doShow"]) == true) unset($queries["ACT_doShow"]);
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = netbk_update_cookie($head, $cookie);
			list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
			
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
				$bankmsgsrsv1 .= "<ACCTID>" . $acctid . "-" . $account["id"] . "</ACCTID>";
				$bankmsgsrsv1 .= "<ACCTTYPE>" . ENV_STR_ACCTTYPE_SAVINGS . "</ACCTTYPE>";
				$bankmsgsrsv1 .= "</BANKACCTFROM>";
				$bankmsgsrsv1 .= "\r\n";
				$bankmsgsrsv1 .= netbk_parse_csv($body, $account);
				$bankmsgsrsv1 .= "</STMTRS>";
				$bankmsgsrsv1 .= "\r\n";
				$bankmsgsrsv1 .= "</STMTTRNRS>";
				$bankmsgsrsv1 .= "\r\n";
			}
		}
		
		$body = $body_old;
	}
	
	$bankmsgsrsv1 .= "</BANKMSGSRSV1>";
	$bankmsgsrsv1 .= "\r\n";
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログアウト") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = netbk_update_cookie($head, $cookie);
		list($head, $body) = netbk_http11($method, $uris, $query, $cookie);
		break;
	}
	
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
function netbk_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.netbk\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function netbk_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function netbk_parse_csv($str, $account) {
	$ret = "";
	$lines = array_reverse(parse_csv($str));
	$cds = array();
	$cds_balamt = "0";
	$cd_date = "";
	$cd_num = 0;
	
	foreach($lines as $line) {
		$cd = array();
		if(count($line) == 6 && $line[0] != "日付") {
			list($cd_name, $cd_dummy) = explode(ENV_CHR_SEPARATOR, str_replace(array("＊", "　"), array(ENV_CHR_SEPARATOR, ENV_CHR_SEPARATOR), $line[1]), 2);
			
			switch($cd_name) {
			case "振込":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
				break;
			case "国税":
			case "地方税":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DEP;
				break;
			case "ＳＢＩハイブリッド預金":
			case "普通":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_XFER;
				break;
			case "振替":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DEBIT;
				break;
			case "利息":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
				break;
			case "ＡＴＭ":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_ATM;
				break;
			default:
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
				break;
			}
			
			// 日付を取得する
			$cd["DTPOSTED"] = parse_date($line[0]);
			
			// 通番を生成する
			if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
			
			// トランザクション番号を生成する
			$cd["FITID"] = $cd["DTPOSTED"] . sprintf("%04d", (integer)$account["accttype"]) . "000" . sprintf("%05d", $cd_num);
			
			// 摘要を取得する
			$cd["NAME"] = mb_convert_string($line[1]);
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)((double)parse_amount($line[3]) - (double)parse_amount($line[2]));
			
			// 残高を取得する
			$cds_balamt = (string)((double)parse_amount($line[4]));
			$cd["MEMO"] = mb_convert_string($line[5]);
			
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
	$ret .= "<BALAMT>" . $cds_balamt . "</BALAMT>";
	$ret .= "<DTASOF>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTASOF>";
	$ret .= "</LEDGERBAL>";
	$ret .= "\r\n";
	
	// 口座名称を出力する
	if($account["acctname"] != "") {
		$ret .= "<MKTGINFO>" . mb_convert_string($account["acctname"]) . "</MKTGINFO>";
		$ret .= "\r\n";
	}
	
	return $ret;
}

?>
