<?php
/*
saisoncard.inc: セゾンカードのCSVよりOFXファイルを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["inputId"];
$pass = $ofxforms["inputPassword"];
$auth = urlencode(mb_convert_encoding(urldecode($ofxforms["secretAnswer"]), "Shift_JIS", "UTF-8"));
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
	list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
	
	// ログイン画面を取得する
	$as = parse_tag($body, "a", true); // 再帰的に取得する
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログインはこちらから") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// ログインする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "_USA01Form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["inputId"] = "inputId=" . $user;
		$queries["inputPassword"] = "inputPassword=" . $pass;
		if(isset($queries["inputIdCheckBox"]) == true) unset($queries["inputIdCheckBox"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
	}
}

if(strpos($body, "秘密の質問によるご本人様確認") !== false) {
	// 秘密の答えを入力する
	$tds = parse_tag($body, "td");
	$shitsumon = "";
	foreach($tds as $td) if($td["class"] == "td-01") {
		$shitsumon = trim(strip_tags($td["innerHTML"]));
		break;
	}
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "_UST00Form");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["secretAnswer"] = "secretAnswer=";
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = saisoncard_update_cookie($head, $cookie);
	}
	
	// セッションを退避する
	$sid = 1;
	$head = "";
	$body = "";
} else if($sid == 1) {
	// セッションを復元する
	$sid = 0;
	$uris = parse_uri($uri);
	$query = str_replace("secretAnswer=", "secretAnswer=" . $auth, $query);
	list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
}

if($sid > 0) {
	// セッションを引き継ぐ
	$resp["status"] = ENV_NUM_STATUS_ADDITION;
	$resp["aid"] = "secretAnswer";
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
} else if(strpos($body, "秘密の質問設定") !== false) {
	// ログイン後の画面が通常と異なる場合
	$resp["status"] = ENV_NUM_STATUS_CAUTION;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "メンテナンス中") !== false || strpos($body, "サービスがご利用いただけません") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
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
	$account = array();
	$account["id"] = 0;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// Netアンサー情報照会画面を取得する
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "Netアンサー登録内容変更 NetアンサーID、パスワード、 メールアドレスなど ") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$tds = parse_tag($body, "td", true); // 再帰的に取得する
	$bufs = array();
	foreach($tds as $td) if($td["class"] == "td-01") array_push($bufs, trim(strip_tags($td["innerHTML"])));
	if(count($bufs) >= 2) {
		// カード名称を取得する
		$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $bufs[0]));
		$account["name"] = $settings["name"];
		
		// カード番号を取得する
		$account["acctid"] = $bufs[2];
	}
	
	// ご利用明細画面を取得する
	$as = parse_tag($body, "a");
	$c = parse_tag_search($as, "innerHTML", "ご利用明細照会");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($as[$c]["href"], $uris);
		$query = "";
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
	}
	
	// 最新の請求月を取得する（お支払日は4日であると仮定する）
	$c = preg_match_all("/([0-9]{1,2})" . preg_quote("月お支払分") . "/", $body, $matches);
	if($c > 0) {
		$matches[1][0] = sprintf("%02d", (integer)$matches[1][0]);
		$account["paydate"] = (date("m") <= $matches[1][0]? date("Y"): (string)((integer)date("Y") + 1)) . $matches[1][0] . "04";
	}
	
	$body_old = $body;
	
	// CSVファイルをダウンロードする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "CSVダウンロード") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	if(strpos($head, "Content-Type: text/html") === false) {
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
		$creditcardmsgsrsv1 .= saisoncard_parse_csv($body, $account);
		$creditcardmsgsrsv1 .= "</CCSTMTRS>";
		$creditcardmsgsrsv1 .= "\r\n";
		$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
		$creditcardmsgsrsv1 .= "\r\n";
		
		$creditcardmsgsrsv1 .= "</CREDITCARDMSGSRSV1>";
		$creditcardmsgsrsv1 .= "\r\n";
	}
	
	$body = $body_old;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$as = parse_tag($body, "a");
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ログアウト") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = saisoncard_update_cookie($head, $cookie);
		list($head, $body) = saisoncard_http11($method, $uris, $query, $cookie);
		break;
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
function saisoncard_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.saisoncard\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function saisoncard_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function saisoncard_parse_csv($str, $account) {
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
			case "今回ご請求額":
				$cds_balamt = (string)(double)parse_amount($line[1]);
				$ledge_balamt = (double)$cds_balamt;
				break;
			case "カード名称":
			default:
				break;
			}
		} else if(count($line) == 7 && $line[0] != "利用日") {
			// PAYMENT固定とする
			$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_PAYMENT;
			
			// 日付を取得する
			$cd["DTPOSTED"] = parse_date($line[0]);
			if($cds_s == "") $cds_s = $cd["DTPOSTED"];
			$cds_e = $cd["DTPOSTED"];
			
			// 通番を生成する
			if($cd["DTPOSTED"] == $cd_date) $cd_num++; else $cd_num = 0;
			
			// トランザクション番号を生成する
			$cd["FITID"] = $cd["DTPOSTED"] . sprintf("%04d", $account["id"]) . substr($account["paydate"], 4, 2) . "0" . sprintf("%05d", $cd_num);
			
			// 摘要を取得する
			$cd["NAME"] = $line[1];
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[5]));
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			// 残高を取得する
			$cd["MEMO"] = ($line[6] != ""? $line[6]: ENV_STR_OFX_MEMO);
			
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
