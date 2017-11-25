<?php
/*
jaccscard.inc: ジャックスカードのCSVよりOFXファイルを生成する
Copyright (C) 2012-2017 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Copyright (C) 2015-2017 OFFICE OUTERGUY. All Rights Reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["IUSERID"];
$pass = $ofxforms["IPASSWORD"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if($a["innerHTML"] == "ログイン") {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = jaccscard_update_cookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
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
	$cookie = jaccscard_update_cookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "MainForm");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["IUSERID"] = "IUSERID=" . $user;
	$queries["IPASSWORD"] = "IPASSWORD=" . $pass;
	$queries["_TRANID"] = "_TRANID=JALG00001_01M";
	if(isset($queries["JALG00001_01M"]) == true) unset($queries["JALG00001_01M"]);
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = jaccscard_update_cookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = jaccscard_update_cookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
	}
}

if(strpos($body, "メンテナンスを行っております") !== false) {
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
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ご利用状況の確認画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=JAMY00001_01M"; // ご利用状況の確認
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jaccscard_update_cookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
	}
	
	// ご利用代金明細を見る画面を取得する
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=JAST00001_21M"; // ご利用代金明細を見る
		$queries["_SUBINDEX"] = "_SUBINDEX=0"; // （カードが複数枚存在する場合）最初のカード
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jaccscard_update_cookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
	}
	
	// お支払日を取得する
	$trs = parse_tag($body, "tr", true); // 再帰的に取得する
	$c = parse_tag_search($trs, "class", "first");
	if($c != -1) {
		$tds = parse_tag($trs[$c]["innerHTML"], "td");
		if(count($tds) >= 1) $account["paydate"] = parse_date(trim(str_replace("\t", "", $tds[0]["innerHTML"])));
	}
	
	// カード名・カード番号を取得する
	$dds = parse_tag($body, "dd");
	if(count($dds) >= 2) {
		$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $dds[0]["innerHTML"]));
		$account["name"] = $settings["name"];
		$account["acctid"] = strip_tags(str_replace("&lowast;", "*", $dds[1]["innerHTML"]));
	}
	
	$body_old = $body;
	
	// CSVファイルをダウンロードする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=JAST00016_23C"; // 明細をCSVで保存する
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jaccscard_update_cookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
		
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
			$creditcardmsgsrsv1 .= jaccscard_parse_csv($body, $account);
			$creditcardmsgsrsv1 .= "</CCSTMTRS>";
			$creditcardmsgsrsv1 .= "\r\n";
			$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
			$creditcardmsgsrsv1 .= "\r\n";
			
			$creditcardmsgsrsv1 .= "</CREDITCARDMSGSRSV1>";
			$creditcardmsgsrsv1 .= "\r\n";
		}
		
	}
	
	$body = $body_old;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["_TRANID"] = "_TRANID=JALG00012_00M"; // ログアウト
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = jaccscard_update_cookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $uris, $query, $cookie);
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
function jaccscard_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.jaccs\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function jaccscard_update_cookie($head, $cookie) {
	return update_cookie(array("JSESSIONID", "[0-9]{17}"), parse_header($head, "set-cookie"), $cookie);
}

function jaccscard_parse_csv($str, $account) {
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
	$flg = false;
	
	foreach($lines as $line) {
		$cd = array();
		
		if(count($line) == 2 && $line[0] == "(A)1回・2回・分割・ボーナス払の今回お支払金額小計") {
			// 今回お支払金額を取得する
			$cds_balamt = (string)((double)parse_amount($line[1]));
		} else if(count($line) == 13 && $line[1] == "＜＜次回以降のお支払明細＞＞") {
			// 次回以降のお支払明細を処理しない
			$flg = true;
			// break;
		} else if(count($line) == 13 && $line[0] != "ご利用年月日" && $line[0] != "" && $flg == false) {
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
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[7]));
			
			// 費目を取得する
			$cd["MEMO"] = ($line[12] != "未設定"? $line[12]: ENV_STR_OFX_MEMO);
			
			// 残高を取得する
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			array_push($cds, $cd);
			$cd_date = $cd["DTPOSTED"];
		}
	}
	
	$ledge_balamt += (double)$cds_balamt;
	
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
