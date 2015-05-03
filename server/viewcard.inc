<?php
/*
viewcard.inc: ビューカードのCSVよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["id"];
$pass = $ofxforms["pass"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(strip_tags($a["innerHTML"]) == "VIEW&#39;s NETログイン（VIEW&#39;s NET IDの方）") {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = viewcard_update_cookie($head, $cookie);
	list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
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
		$cookie = viewcard_update_cookie($head, $cookie);
		list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
	}
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "target", "_self");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["id"] = "id=" . $user;
	$queries["pass"] = "pass=" . $pass;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = viewcard_update_cookie($head, $cookie);
	list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = viewcard_update_cookie($head, $cookie);
		list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
	}
}

if(strpos($body, "前回ログイン") === false) {
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
	// ご利用明細照会（お支払方法の変更）画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "alt", "ご利用明細照会（お支払方法の変更）");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = viewcard_update_cookie($head, $cookie);
		list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = viewcard_update_cookie($head, $cookie);
			list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
		}
	}
	
	$accounts = array();
	
	// カード数を取得する
	$selects = parse_tag($body, "select");
	$c = parse_tag_search($selects, "name", "DdlCardNO");
	if($c != -1) {
		$options = parse_tag($selects[$c]["innerHTML"], "option");
		for($i = 0; $i < count($options); $i++) {
			$accounts[$i]["id"] = $options[$i]["value"];
			$accounts[$i]["ym"] = $ym;
			if(preg_match("/(.*?)[\s\t]*([0-9\*\-]+)/", trim(strip_tags($options[$i]["innerHTML"])), $matches) > 0) {
				// カード名称を取得する
				$accounts[$i]["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $matches[1]));
				
				$accounts[$i]["name"] = $settings["name"];
				
				// カード番号を取得する
				$accounts[$i]["acctid"] = $matches[2];
			}
		}
	}
	
	$body_old = $body;
	
	$creditcardmsgsrsv1 = "";
	$creditcardmsgsrsv1 .= "<CREDITCARDMSGSRSV1>";
	$creditcardmsgsrsv1 .= "\r\n";
	
	// カード数分ループする
	foreach($accounts as $account) {
		$body = $body_old;
		
		// 実行時間（タイムアウト）を再設定する
		@set_time_limit(ENV_NUM_TIMEOUT);
		
		// 確定済みの最新明細画面を取得する
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "Frm002");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$queries["DdlCardNO"] = "DdlCardNO=" . $account["id"];
			$queries["__EVENTTARGET"] = "__EVENTTARGET=LnkClaimYm1"; // 確定済みの最新明細
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = viewcard_update_cookie($head, $cookie);
			list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
		}
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY) {
			$locations = parse_header($head, "location");
			if(count($locations) > 0) {
				$method = "GET";
				$uris = parse_uri($locations[0], $uris);
				$query = "";
				$cookie = viewcard_update_cookie($head, $cookie);
				list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
			}
		}
		
		// お支払日は4日であると仮定する
		if(strpos($body, "今月分のご請求（カード年会費は除く）はありません。") !== false) {
			$body = "";
			$account["paydate"] = substr(ENV_STR_DATE_TODAY, 0, 6) . "04";
		}
		
		$body_old = $body;
		
		// 明細CSVをダウンロードする
		$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "Form1");
		if($c != -1) {
			$inputs = parse_tag($forms[$c]["innerHTML"], "input");
			$queries = array();
			foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
			$queries["DdlCardNO"] = "DdlCardNO=" . $account["id"];
			$queries["BtnCsvDownloadTop.x"] = "BtnCsvDownloadTop.x=1";
			$queries["BtnCsvDownloadTop.y"] = "BtnCsvDownloadTop.y=1";
			if(isset($queries["BtnCsvDownloadTop"]) == true) unset($queries["BtnCsvDownloadTop"]);
			if(isset($queries["BtnCsvDownloadBottom"]) == true) unset($queries["BtnCsvDownloadBottom"]);
			if(isset($queries["BtnPdfDownloadTop"]) == true) unset($queries["BtnPdfDownloadTop"]);
			if(isset($queries["BtnPdfDownloadBottom"]) == true) unset($queries["BtnPdfDownloadBottom"]);
			if(isset($queries["BtnList"]) == true) unset($queries["BtnList"]);
			
			$method = $forms[$c]["method"];
			$uris = parse_uri($forms[$c]["action"], $uris);
			$query = implode("&", $queries);
			$cookie = viewcard_update_cookie($head, $cookie);
			list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
			
			if(strpos($head, "Content-Type: text/html") === false) {
				$creditcardmsgsrsv1 .= "<CCSTMTTRNRS>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "<TRNUID>0</TRNUID>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "<STATUS><CODE>0</CODE><SEVERITY>INFO</SEVERITY></STATUS>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "<CCSTMTRS>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "<CURDEF>" . ENV_STR_OFX_CURRENCY_JPY . "</CURDEF>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "<CCACCTFROM>";
				$creditcardmsgsrsv1 .= "<ACCTID>" . $account["acctid"] . "</ACCTID>";
				$creditcardmsgsrsv1 .= "</CCACCTFROM>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= viewcard_parse_csv($body, $account);
				$creditcardmsgsrsv1 .= "</CCSTMTRS>";
				$creditcardmsgsrsv1 .= "\r\n";
				$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
				$creditcardmsgsrsv1 .= "\r\n";
			}
		}
		
		$body = $body_old;
	}
	
	$creditcardmsgsrsv1 .= "</CREDITCARDMSGSRSV1>";
	$creditcardmsgsrsv1 .= "\r\n";
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "ログアウト"); // ログアウト
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = viewcard_update_cookie($head, $cookie);
		list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$forms = parse_tag($body, "form");
		$c = parse_tag_search($forms, "name", "frmMain");
		if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["Logout.x"] = "Logout.x=0";
		$queries["Logout.y"] = "Logout.y=0";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = viewcard_update_cookie($head, $cookie);
		list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = viewcard_update_cookie($head, $cookie);
			list($head, $body) = viewcard_http11($method, $uris, $query, $cookie);
		}
	}
	
	// OFXファイルを出力する
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	if(strpos($creditcardmsgsrsv1, "<BANKTRANLIST>") === false) {
		// 明細が存在しない場合
		$resp["ofx"] = generate_ofx($resp["status"]);
	} else {
		// 明細が存在する場合
		$resp["ofx"] = generate_ofx($resp["status"], $creditcardmsgsrsv1);
	}
}
return $resp;

// HTTP/1.1
function viewcard_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/^viewsnet\.jp$/", $uris["host"]) > 0 || (strtoupper($method) == "GET" && preg_match("/\.jreast\.co\.jp$/", $uris["host"]) > 0)) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function viewcard_update_cookie($head, $cookie) {
	return update_cookie(array("BIGipServerPL-VNWEB-PC_WAF-TCP80", "VIEWSNET_WEB", "ASP.NET_SessionId", "citrix_ns_id", "citrix_ns_id_.viewsnet.jp_%2F_wat"), parse_header($head, "set-cookie"), $cookie);
}

function viewcard_parse_csv($str, $account) {
	$ret = "";
	$lines = parse_csv(mb_convert_string($str));
	$cds = array();
	$cds_balamt = "0";
	$cds_paydate = $account["paydate"];
	$cds_s = "";
	$cds_e = "";
	$cd_date = "";
	$cd_num = 0;
	$ledge_balamt = 0;
	
	foreach($lines as $line) {
		$cd = array();
		
		if(count($line) == 2) {
			switch($line[0]) {
			case "お支払日":
				$cds_paydate = parse_date($line[1]);
				break;
			case "今回お支払金額":
				$cds_balamt = (string)(double)parse_amount($line[1]);
				$ledge_balamt = (double)$cds_balamt;
				break;
			case "会員番号":
			case "対象カード":
			default:
				break;
			}
		} else if(count($line) == 11 && $line[0] != "ご利用年月日") {
			
			// 日付を取得する
			$cd["DTPOSTED"] = parse_date($line[0]);
			if($line[1] == "ショッピングリボ弁済金") {
				$cd["DTPOSTED"] = $cds_paydate;
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
			} else {
				// PAYMENT固定とする
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_PAYMENT;
			}
			if($cds_s == "") $cds_s = $cd["DTPOSTED"];
			$cds_e = $cd["DTPOSTED"];
			
			// 通番を生成する
			if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
			
			// トランザクション番号を生成する
			$cd["FITID"] = $cd["DTPOSTED"] . sprintf("%04d", $account["id"]) . substr($account["paydate"], 4, 2) . "0" . sprintf("%05d", $cd_num);
			
			// 摘要を取得する
			$cd["NAME"] = $line[1];
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[7]));
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			// 残高を取得する
			$cd["MEMO"] = ($line[5] != ""? $line[5]: ENV_STR_OFX_MEMO);
			
			array_push($cds, $cd);
			$cd_date = $cd["DTPOSTED"];
		}
	}
	
	if($cds_s == "") $cds_s = ENV_STR_DATE_TODAY;
	if($cds_e == "") $cds_e = ENV_STR_DATE_TODAY;
	if($cds_s > $cds_e) $cds_e = $cds_s;
	
	// クレジットカード支払請求を明細に追加する
	$i = count($cds);
	$cds[$i]["DTPOSTED"] = $cds_paydate;
	$cds[$i]["NAME"] = $account["name"];
	$cds[$i]["MEMO"] = ENV_STR_OFX_MEMO;
	$cds[$i]["TRNAMT"] = $cds_balamt;
	$cds[$i]["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
	$cds[$i]["FITID"] = $cds[$i]["DTPOSTED"] . sprintf("%04d", $account["id"]) . substr($account["paydate"], 4, 2) . "100000";
	
	// BANKTRANLIST
	$ret .= "<BANKTRANLIST>";
	$ret .= "\r\n";
	$ret .= "<DTSTART>" . $cds_s . ENV_STR_OFX_TZ . "</DTSTART>";
	$ret .= "<DTEND>" . $cds_e . ENV_STR_OFX_TZ . "</DTEND>";
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
	
	// 支払後残高を出力する
	$ret .= "<LEDGERBAL>";
	$ret .= "<BALAMT>" . (string)$ledge_balamt . "</BALAMT>";
	$ret .= "<DTASOF>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTASOF>";
	$ret .= "</LEDGERBAL>";
	$ret .= "\r\n";
	
	// カード名称を出力する
	if($account["acctname"] != "") {
		$ret .= "<MKTGINFO>" . $account["acctname"] . "</MKTGINFO>";
		$ret .= "\r\n";
	}
	
	return $ret;
}

?>
