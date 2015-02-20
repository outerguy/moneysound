<?php
/*
smbccard.inc: 三井住友カードのCSVよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["userid"];
$pass = $ofxforms["password"];

$resp = array();

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	}
}

// ログイン画面を取得する
$as = parse_tag($body, "a");

foreach($as as $a) if(strip_tags($a["innerHTML"]) == "カード会員の方 Vpassログイン") {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = smbccard_update_cookie($head, $cookie);
	list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	break;
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "InForm");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["userid"] = "userid=" . $user;
	$queries["password"] = "password=" . $pass;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = smbccard_update_cookie($head, $cookie);
	list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	}
}

if(strpos($body, "システムメンテナンス中") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "IDまたはパスワードが無効となっております") !== false) {
	// ログイン後の画面が通常と異なる場合
	$resp["status"] = ENV_NUM_STATUS_CAUTION;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "ログイン中") === false) {
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
	$account["flag"] = false;
	$account["paydate"] = "00000000";
	$account["balamt"] = "0";
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	if(strpos($body, "パスワード変更に関するお願い") !== false) {
		// パスワード変更に関するお願い画面の場合、ログイン中　カード会員の方TOP画面を取得する
		$as = parse_tag($body, "a");
		foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログイン中　カード会員の方TOP") != -1) {
			$method = "GET";
			$uris = parse_uri($a["href"], $uris);
			$query = "";
			$cookie = smbccard_update_cookie($head, $cookie);
			list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
			break;
		}
	}
	
	// カード名称を取得する
	$fonts = parse_tag($body, "font");
	$c = parse_tag_search($fonts, "color", "#006633");
	if($c != -1) {
		$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], trim(strip_tags($fonts[$c]["innerHTML"]))));
		$account["name"] = $settings["name"];
	}
	
	// ご利用明細画面を取得する
	// 最新月の確定済みの明細が0件の場合、カード番号を取得できないため、デフォルトで表示される（最新月以前で明細が1件以上ある月の）ご利用明細画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ご利用明細を見る") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// カード番号を取得する
	$trs = parse_tag($body, "tr", true); // 再帰的に取得する
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 2 && $tds[0]["class"] == "sdbc2") {
			$account["acctid"] = trim(strip_tags($tds[0]["innerHTML"]));
			break;
		}
	}
	if($account["acctid"] == "") $account["acctid"] = $user;
	
	// お支払いについて画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "お支払いについて") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// お支払い金額照会画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "お支払い金額照会");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	}
	
	// （最新月の確定済みの）ご利用明細画面を取得する
	$trs = parse_tag($body, "tr", true); // 再帰的に取得する
	$uri = "";
	foreach($trs as $tr) {
		$tds = parse_tag($tr["innerHTML"], "td");
		if(count($tds) == 3 && strpos($tds[0]["innerHTML"], "確定分") !== false) {
			// お支払日を取得する
			if(preg_match("/([0-9]{1,2})" . preg_quote("月") . "([0-9]{1,2})" . preg_quote("日お支払い　確定分") . "/", trim(strip_tags($tds[0]["innerHTML"])), $matches) > 0) {
				$matches[1] = sprintf("%02d", (integer)$matches[1]);
				$matches[2] = sprintf("%02d", (integer)$matches[2]);
				$account["paydate"] = (date("m") <= $matches[1]? date("Y"): (string)((integer)date("Y") + 1)) . $matches[1] . $matches[2];
			}
			
			// お支払合計額を取得する
			$account["balamt"] = parse_amount(strip_tags($tds[1]["innerHTML"]));
			
			$as = parse_tag($tds[2]["innerHTML"], "a");
			if(count($as) > 0 && parse_tag_search(parse_tag($as[0]["innerHTML"], "img"), "alt", "") != -1) $uri = $as[0]["href"];
			// breakしない
		}
	}
	if($uri != "") {
		$method = "GET";
		$uris = parse_uri($uri, $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	}
	
	$body_old = $body;
	
	// CSVファイルをダウンロードする
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "CSV形式で保存");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
	}
	
	$creditcardmsgsrsv1 = "";
	$creditcardmsgsrsv1 .= "<CREDITCARDMSGSRSV1>";
	$creditcardmsgsrsv1 .= "\r\n";
	
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
	$creditcardmsgsrsv1 .= smbccard_parse_csv($body, $account); // 0固定？（要検討）
	$creditcardmsgsrsv1 .= "</CCSTMTRS>";
	$creditcardmsgsrsv1 .= "\r\n";
	$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
	$creditcardmsgsrsv1 .= "\r\n";
	
	$creditcardmsgsrsv1 .= "</CREDITCARDMSGSRSV1>";
	$creditcardmsgsrsv1 .= "\r\n";
	
	$body = $body_old;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログアウト") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = smbccard_update_cookie($head, $cookie);
		list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
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
			$cookie = smbccard_update_cookie($head, $cookie);
			list($head, $body) = smbccard_http11($method, $uris, $query, $cookie);
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
function smbccard_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.smbc-card\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function smbccard_update_cookie($head, $cookie) {
	return update_cookie(array("vpasssession", "pass", "ch"), parse_header($head, "set-cookie"), $cookie);
}

function smbccard_parse_csv($str, $account) {
	$ret = "";
	$lines = array_reverse(parse_csv($str));
	$cds = array();
	$cds_balamt = "0";
	$cds_s = "";
	$cds_e = "";
	$cd_date = "";
	$cd_num = 0;
	
	// 今回お支払金額を取得する
	$ledge_balamt = (double)$account["balamt"];
	
	foreach($lines as $line) {
		$cd = array();
		
		if(count($line) == 7 && $line[0] != "") {
			// WEB明細書CSVの場合
			
			// PAYMENT固定とする
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_PAYMENT;
			
			// 日付を取得する
			$cd["DTPOSTED"] = parse_date($line[0]);
			if($cds_s == "") $cds_s = $cd["DTPOSTED"];
			$cds_e = $cd["DTPOSTED"];
			
			// 通番を生成する
			if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
			
			// トランザクション番号を生成する
			$cd["FITID"] = $cd["DTPOSTED"] . "0000" . substr($account["paydate"], 4, 2) . "0" . sprintf("%05d", $cd_num);
			
			// 摘要を取得する
			$cd["NAME"] = mb_convert_string($line[1]);
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[5]));
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			// 残高を取得する
			$cd["MEMO"] = ($line[7] != ""? mb_convert_string($line[7]): ENV_STR_OFX_MEMO);
			
			array_push($cds, $cd);
			$cd_date = $cd["DTPOSTED"];
		} else if(count($line) == 13) {
			// ご利用明細照会CSVの場合
			
			// PAYMENT固定とする
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_PAYMENT;
			
			// 日付を取得する
			$cd["DTPOSTED"] = parse_date($line[0]);
			if($cds_s == "") $cds_s = $cd["DTPOSTED"];
			$cds_e = $cd["DTPOSTED"];
			
			// 通番を生成する
			if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
			
			// トランザクション番号を生成する
			$cd["FITID"] = $cd["DTPOSTED"] . "0000" . substr($account["paydate"], 4, 2) . "0" . sprintf("%05d", $cd_num);
			
			// 摘要を取得する
			$cd["NAME"] = mb_convert_string($line[1]);
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[6]));
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			// 残高を取得する
			$cd["MEMO"] = ($line[3] != ""? mb_convert_string($line[3]): ENV_STR_OFX_MEMO);
			
			array_push($cds, $cd);
			$cd_date = $cd["DTPOSTED"];
		}
	}
	
	if($cds_s == "") $cds_s = ENV_STR_DATE_TODAY;
	if($cds_e == "") $cds_e = ENV_STR_DATE_TODAY;
	if($cds_s > $cds_e) $cds_e = $cds_s;
	
	// クレジットカード支払請求を明細に追加する
	$i = count($cds);
	$cds[$i]["DTPOSTED"] = $account["paydate"];
	$cds[$i]["NAME"] = mb_convert_string($account["name"]);
	$cds[$i]["MEMO"] = ENV_STR_OFX_MEMO;
	$cds[$i]["TRNAMT"] = $account["balamt"];
	$cds[$i]["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
	$cds[$i]["FITID"] = $cds[$i]["DTPOSTED"] . "0000" . substr($account["paydate"], 4, 2) . "100000";
	
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
