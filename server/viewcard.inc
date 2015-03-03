<?php
/*
viewcard.inc: ビューカードのHTMLよりOFXファイルを生成する
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
		$creditcardmsgsrsv1 .= viewcard_parse_html($body, $account);
		$creditcardmsgsrsv1 .= "</CCSTMTRS>";
		$creditcardmsgsrsv1 .= "\r\n";
		$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
		$creditcardmsgsrsv1 .= "\r\n";
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

function viewcard_parse_html($body, $account) {
	$ret = "";
	$cds = array();
	$cds_s = "";
	$cds_e = "";
	$cd_date = "";
	$cd_num = 0;
	
	$tables = parse_tag($body, "table");
	
	// お支払日・お支払総額を取得する
	$c = parse_tag_search($tables, "summary", "お支払総額。");
	if($c != -1) {
		// お支払日を取得する
		$spans = parse_tag($tables[$c]["innerHTML"], "span");
		$cds_paydate = (count($spans) == 1? parse_date(str_replace("お支払日：", "", $spans[0]["innerHTML"])): $account["paydate"]);
		
		// お支払総額を取得する
		$tds = parse_tag($tables[$c]["innerHTML"], "td");
		$cds_balamt = (count($tds) == 1? parse_amount(strip_tags($tds[0]["innerHTML"])): "0");
		$ledge_balamt = (double)$cds_balamt;
	}
	
	// 明細行を含むtableを取得する
	$c = parse_tag_search($tables, "summary", "ご利用年月日。");
	if($c != -1) {
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		
		// 明細行毎に処理する
		for($i = 0; $i < count($trs); $i++) {
			$tds = parse_tag($trs[$i]["innerHTML"], "td");
			if(count($tds) == 12) {
				// ご利用年月日を取得する
				$cds[$i]["DTPOSTED"] = parse_date(trim(strip_tags(str_replace("<br/>", "/", $tds[0]["innerHTML"]))));
				if($cds_s == "") $cds_s = $cds[$i]["DTPOSTED"];
				$cds_e = $cds[$i]["DTPOSTED"];
				
				// ご利用箇所を取得する
				$cds[$i]["NAME"] = mb_convert_string(trim($tds[2]["innerHTML"]));
				
				// 今回ご請求額を取得する
				$spans = parse_tag($tds[8]["innerHTML"], "span");
				$c = parse_tag_search($spans, "id", "RtUseInfoList__ctl1_LblTimMon");
				if($c != -1) {
					$cds[$i]["TRNAMT"] = (string)(-1 * parse_amount($spans[$c]["innerHTML"]));
					$ledge_balamt += (double)$cds[$i]["TRNAMT"];
				}
				
				// 備考を取得する
				$memo = mb_convert_string(trim(strip_tags($tds[6]["innerHTML"])));
				$cds[$i]["MEMO"] = ($memo != ""? $memo: ENV_STR_OFX_MEMO);
				
				// PAYMENT固定とする
				$cds[$i]["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_PAYMENT;
				
				// 通番を生成する
				if($cds[$i]["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
				$cd_date = $cds[$i]["DTPOSTED"];
				
				// トランザクション番号を生成する
				$cds[$i]["FITID"] = $cds[$i]["DTPOSTED"] . sprintf("%04d", $account["id"]) . substr($cds_paydate, 4, 2) . "0" . sprintf("%05d", $cd_num);
			}
		}
	}
	
	if($cds_s == "") $cds_s = ENV_STR_DATE_TODAY;
	if($cds_e == "") $cds_e = ENV_STR_DATE_TODAY;
	if($cds_s > $cds_e) $cds_e = $cds_s;
	
	// クレジットカード支払請求を明細に追加する
	$cds[$i]["DTPOSTED"] = $cds_paydate;
	$cds[$i]["NAME"] = mb_convert_string($account["name"]);
	$cds[$i]["MEMO"] = ENV_STR_OFX_MEMO;
	$cds[$i]["TRNAMT"] = $cds_balamt;
	$cds[$i]["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
	$cds[$i]["FITID"] = $cds[$i]["DTPOSTED"] . sprintf("%04d", $account["id"]) . substr($cds_paydate, 4, 2) . "100000";
	
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
		$ret .= "<MKTGINFO>" . mb_convert_string($account["acctname"]) . "</MKTGINFO>";
		$ret .= "\r\n";
	}
	
	return $ret;
}

?>
