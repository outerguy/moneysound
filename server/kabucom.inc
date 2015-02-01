<?php
/*
kabucom.inc: カブドットコム証券のCSVよりOFXファイルを生成する
Copyright (C) 2010-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["SsLogonUser"];
$pass = $ofxforms["SsLogonPassword"];

$resp = array();

define("ENV_STR_ASSET_STOCK_BUY", "買");
define("ENV_STR_ASSET_STOCK_SELL", "売");
define("ENV_STR_ASSET_FUND_BUY", "買付");
define("ENV_STR_ASSET_FUND_SELL", "解約");

$acctid = mb_convert_string($user);

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$urls = parse_url($settings["home"]);
$method = "GET";
$protocol = ($urls["scheme"] != ""? $urls["scheme"]: "");
$host = ($urls["host"] != ""? $urls["host"]: "");
$page = ($urls["path"] != ""? $urls["path"]: "/");
$query = ($urls["query"] != ""? $urls["query"]: "");
$cookie = "";
list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(trim(strip_tags($a["innerHTML"])) == "ログイン") {
	$urls = parse_url($a["href"]);
	$method = "GET";
	$protocol = ($urls["scheme"] != ""? $urls["scheme"]: "");
	$host = ($urls["host"] != ""? $urls["host"]: "");
	$page = ($urls["path"] != ""? $urls["path"]: "/");
	$query = ($urls["query"] != ""? $urls["query"]: "");
	$cookie = "";
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	break;
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY && preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $head, $matches) > 0) {
	$method = "GET";
	$urls = parse_url($matches[1]);
	if($urls["scheme"] != "") $protocol = $urls["scheme"];
	if($urls["host"] != "") $host = $urls["host"];
	if($urls["path"] != "") $page = $urls["path"];
	$query = ($urls["query"] != ""? $urls["query"]: "");
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
}

// Light版画面を取得する
if(preg_match("/<[Aa].*?[Hh][Rr][Ee][Ff][\s\t]*\=[\s\t]*\"([^\"]*)\"[^>]*>[\s\t\r\n]*<[Ii][Mm][Gg].*?[Aa][Ll][Tt][\s\t]*\=[\s\t]*\"" . preg_quote("Light（簡易版）") . "\"[^>]*><\/[Aa]>/", $body, $matches) > 0) {
	$method = "GET";
	$urls = parse_url($matches[1]);
	if($urls["scheme"] != "") $protocol = $urls["scheme"];
	if($urls["host"] != "") $host = $urls["host"];
	if($urls["path"] != "") $page = $urls["path"];
	if($urls["query"] != "") $query = $urls["query"];
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// リダイレクトする
	if(preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $head, $matches) > 0) {
		$method = "GET";
		$urls = parse_url($matches[1]);
		if($urls["scheme"] != "") $protocol = $urls["scheme"];
		if($urls["host"] != "") $host = $urls["host"];
		if($urls["path"] != "") $page = $urls["path"];
		$query = ($urls["query"] != ""? $urls["query"]: "");
		$cookie = kabucom_updatecookie($head, $cookie);
		list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	}
}

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "xyz");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["SsLogonHost"] = "SsLogonHost=100"; // 主系
	$queries["SsLogonUser"] = "SsLogonUser=" . $user;
	$queries["SsLogonPassword"] = "SsLogonPassword=" . $pass;
	
	$method = $forms[$c]["method"];
	$urls = parse_url($forms[$c]["action"]);
	if($urls["scheme"] != "") $protocol = $urls["scheme"];
	if($urls["host"] != "") $host = $urls["host"];
	if($urls["path"] != "") $page = $urls["path"];
	if($urls["query"] != "") $query = $urls["query"];
	$query = implode("&", $queries);
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY && preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $head, $matches) > 0) {
		$method = "GET";
		$urls = parse_url($matches[1]);
		if($urls["scheme"] != "") $protocol = $urls["scheme"];
		if($urls["host"] != "") $host = $urls["host"];
		if($urls["path"] != "") $page = $urls["path"];
		$query = ($urls["query"] != ""? $urls["query"]: "");
		$cookie = kabucom_updatecookie($head, $cookie);
		list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	}
}

// 重要なお知らせ画面の文字コードを強制的に変換する
if(preg_match("/<[Mm][Ee][Tt][Aa].*?[Cc][Oo][Nn][Tt][Ee][Nn][Tt][\s\t]*=[\s\t]*([\'\"]?).*?[Cc][Hh][Aa][Rr][Ss][Ee][Tt][\s\t]*\=[\s\t]*x\-sjis\\1[^>]*>/", $body, $matches) > 0) $body_old = mb_convert_encoding($body, "Shift_JIS", "UTF-8");

// 重要なお知らせ画面をスキップする
if(strpos($body_old, "－重要なお知らせ－") !== false) {
	if(preg_match("/<[Ff][Oo][Rr][Mm].*?[Aa][Cc][Tt][Ii][Oo][Nn][\s\t]*=[\s\t]*\"([^\"]*)\"[^>]*>/", $body_old, $matches) > 0) {
		$method = "POST";
		$urls = parse_url($matches[1]);
		if($urls["scheme"] != "") $protocol = $urls["scheme"];
		if($urls["host"] != "") $host = $urls["host"];
		if($urls["path"] != "") $page = $urls["path"];
		if($urls["query"] != "") $query = $urls["query"];
		
		$c = preg_match_all("/<[Ii][Nn][Pp][Uu][Tt].*?[Nn][Aa][Mm][Ee][\s\t]*=[\s\t]*([\'\"])([^\'\"]+)\\1.*?[Vv][Aa][Ll][Uu][Ee][\s\t]*=[\s\t]*([\'\"])([^\'\"]*)\\3[^>]*>/", $body_old, $matches);
		$queries = array();
		for($i = 0; $i < $c; $i++) $queries[$matches[2][$i]] = urlencode($matches[2][$i]) . "=" . urlencode($matches[4][$i]);
		$query = implode("&", $queries);
		$cookie = kabucom_updatecookie($head, $cookie);
		list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
		
		// リダイレクトする
		$retry = 0;
		while($retry++ < ENV_NUM_HTTP11_RETRY && preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $head, $matches) > 0) {
			$method = "GET";
			$urls = parse_url($matches[1]);
			if($urls["scheme"] != "") $protocol = $urls["scheme"];
			if($urls["host"] != "") $host = $urls["host"];
			if($urls["path"] != "") $page = $urls["path"];
			$query = ($urls["query"] != ""? $urls["query"]: "");
			$cookie = kabucom_updatecookie($head, $cookie);
			list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
		}
	}
}

if(strpos($body, "ただいまサービスをご利用になれません") !== false || strpos($body, "現在メンテナンス中") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if($retry == ENV_NUM_HTTP11_RETRY || strpos($body, "お客さまへのお知らせ") === false) {
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
	$investments = array();
	$cts = array();
	$ct_date = "";
	$ct_num = 0;
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 現物株式を取得する
	$method = "GET";
	$page = "/Light/Account/Accountstatus/AccountStatus.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	$query = "";
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// 現物株式CSVを取得する
	$page = "/Light/Account/Accountstatus/AccountStatusCsv.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// 銘柄一覧を生成する
	$lines = parse_csv($body);
	foreach($lines as $line) {
		if(count($line) == 12 && $line[0] != "銘柄名") {
			// 株式
			array_push($investments, array(ENV_STR_OFX_STOCK, rtrim($line[1]), mb_convert_string($line[0]), mb_convert_uniquename($line[0]), $line[3], $line[4], $line[6], $line[8]));
		} else {
			// その他（処理対象外）
		}
	}
	
	// 投資信託を取得する
	$method = "GET";
	$page = "/Light/Account/Accountstatus/AccountStatusToshin.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// 投資信託CSVを取得する
	$page = "/Light/Account/Accountstatus/AccountStatusToshinCsv.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// 銘柄一覧を生成する
	$lines = parse_csv($body);
	foreach($lines as $line) {
		if(count($line) == 11 && $line[0] != "ファンド名") {
			// 投資信託
			array_push($investments, array(ENV_STR_OFX_FUND, "", mb_convert_string($line[0]), mb_convert_uniquename($line[0]), $line[3], $line[2], $line[7], $line[6]));
		} else {
			// その他（処理対象外）
		}
	}
	
	$invposlist = "";
	$seclist = "";
	foreach($investments as $investment) {
		$cl = array();
		// INVPOSLIST
		$cl["CATEGORY"] = $investment[0];
		$cl["SECNAME"] = $investment[2];
		$cl["HELDINACCT"] = ENV_STR_OFX_CASH;
		$cl["UNITS"] = (double)$investment[4];
		$cl["POSTYPE"] = "LONG";
		$cl["MKTVAL"] = (double)$investment[6];
		$cl["PL"] = (string)((double)$investment[7]);
		$cl["DTPRICEASOF"] = ENV_STR_DATE_TODAY;
		switch($cl["CATEGORY"]) {
		case ENV_STR_OFX_STOCK:
			$cl["UNIQUEIDTYPE"] = ENV_STR_OFX_CODE_STOCK;
			$cl["UNIQUEID"] = $investment[1];
			$cl["UNITPRICE"] = (double)$investment[5];
			break;
		case ENV_STR_OFX_FUND:
			$cl["UNIQUEIDTYPE"] = ENV_STR_OFX_CODE_HC;
			$cl["UNIQUEID"] = mb_convert_uniqueid($investment[3]);
			$cl["UNITPRICE"] = round((double)$investment[5] / round((double)$investment[4] * (double)$investment[5] / (double)$investment[6], 0), 4);
			break;
		default:
			$cl["CATEGORY"] = "";
			break;
		}
		if($cl["CATEGORY"] != "") {
			// INVPOSLIST
			$invposlist .= "<POS" . $cl["CATEGORY"] . "><INVPOS>";
			$invposlist .= "<SECID><UNIQUEID>" . $cl["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $cl["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$invposlist .= "<HELDINACCT>" . $cl["HELDINACCT"] . "</HELDINACCT>";
			$invposlist .= "<POSTYPE>" . $cl["POSTYPE"] . "</POSTYPE>";
			$invposlist .= "<UNITS>" . $cl["UNITS"] . "</UNITS>";
			$invposlist .= "<UNITPRICE>" . $cl["UNITPRICE"] . "</UNITPRICE>";
			$invposlist .= "<MKTVAL>" . $cl["MKTVAL"] . "</MKTVAL>";
			$invposlist .= "<DTPRICEASOF>" . $cl["DTPRICEASOF"] . ENV_STR_OFX_TZ . "</DTPRICEASOF>";
			$invposlist .= "<MEMO>" . $cl["PL"] . "</MEMO>";
			$invposlist .= "</INVPOS></POS" . $cl["CATEGORY"] . ">";
			$invposlist .= "\r\n";
			
			// SECLIST
			$seclist .= "<" . $cl["CATEGORY"] . "INFO><SECINFO>";
			$seclist .= "<SECID><UNIQUEID>" . $cl["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $cl["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$seclist .= "<SECNAME>" . $cl["SECNAME"] . "</SECNAME>";
			$seclist .= "</SECINFO></" . $cl["CATEGORY"] . "INFO>";
			$seclist .= "\r\n";
		}
	}
	
	// お預り金を取得する（画面遷移が必要なため）
	$page = "/Light/Support/Rule/TradeLimit.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// 現物株式お取引履歴CSVを取得する
	$page = "/Light/Account/TradeRireki/TradeRirekiKabuCsv.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	$lines = parse_csv($body);
	foreach($lines as $line) {
		if(count($line) == 13 && $line[0] != "約定日") {
			// 種別、銘柄、売買を取得する
			$ct_category = ENV_STR_OFX_STOCK;
			$ct_uniqueid = mb_convert_string(rtrim($line[4]));
			$ct_secname = "";
			$ct_uniqueidtype = ENV_STR_OFX_CODE_STOCK;
			// 銘柄一覧より検索する
			foreach($investments as $investment) {
				if($investment[0] == ENV_STR_OFX_STOCK && $investment[1] == $ct_uniqueid) {
					// 持ち高がある場合
					$ct_secname = mb_convert_string($investment[2]);
					break;
				}
			}
			if($ct_secname == "") {
				// 持ち高がない場合
				$ct_secname = mb_convert_string($line[3]);
				
				// SECLIST
				if(strpos($seclist, $ct_uniqueid) === false) {
					$seclist .= "<!-- SOLD -->";
					$seclist .= "<" . $ct_category . "INFO><SECINFO>";
					$seclist .= "<SECID><UNIQUEID>" . $ct_uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $ct_uniqueidtype . "</UNIQUEIDTYPE></SECID>";
					$seclist .= "<SECNAME>" . $ct_secname . "</SECNAME>";
					$seclist .= "</SECINFO></" . $ct_category . "INFO>";
					$seclist .= "\r\n";
				}
			}
			// 単価を取得する
			$ct_unitprice = (double)$line[7];
			// 手数料を取得する
			$ct_fees = (integer)$line[9];
			// 税を取得する
			$ct_taxes = (integer)$line[10];
			switch($line[5]) {
			case ENV_STR_ASSET_STOCK_SELL:
				$ct_buytype = ENV_STR_OFX_SELL;
				$ct_units = -1;
				$ct_total = 1;
				break;
			case ENV_STR_ASSET_STOCK_BUY:
			default:
				$ct_buytype = ENV_STR_OFX_BUY;
				$ct_units = 1;
				$ct_total = -1;
				break;
			}
			// 数量を取得する
			$ct_units *= (integer)$line[6];
			// 金額を取得する
			$ct_total *= (integer)$line[8];
			// 約定日を取得する
			$ct_dttrade = parse_date($line[0]);
			// 通番を生成する
			if($ct_dttrade == $ct_date) $ct_num++; else $ct_num = 0;
			// トランザクション番号を生成する
			$ct_fitid = $ct_dttrade . "0000010" . sprintf("%05d", $ct_num);
			
			$ct = array();
			$ct["CATEGORY"] = $ct_category;
			$ct["FITID"] = $ct_fitid;
			$ct["DTTRADE"] = $ct_dttrade;
			$ct["UNIQUEID"] = $ct_uniqueid;
			$ct["SECNAME"] = $ct_secname;
			$ct["UNIQUEIDTYPE"] = $ct_uniqueidtype;
			$ct["UNITS"] = (string)$ct_units;
			$ct["UNITPRICE"] = (string)$ct_unitprice;
			// OFX仕様上、COMMISSION、FEES、TAXESはマイナス値を許容しない
			// $ct["COMMISSION"] = (string)abs($ct_commission);
			$ct["FEES"] = (string)abs($ct_fees);
			$ct["TAXES"] = (string)abs($ct_taxes);
			$ct["TOTAL"] = (string)$ct_total;
			$ct["SUBACCTSEC"] = ENV_STR_OFX_CASH;
			$ct["SUBACCTFUND"] = ENV_STR_OFX_CASH;
			$ct["BUYTYPE"] = $ct_buytype;
			array_push($cts, $ct);
			
			$ct_date = $ct_dttrade;
		}
	}
	
	// 投資信託お取引履歴CSVを取得する
	$page = "/Light/Account/TradeRireki/TradeRirekiToshinCsv.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	$lines = parse_csv($body);
	$ct_date = "";
	$ct_num = 0;
	foreach($lines as $line) {
		if(count($line) == 14 && $line[0] != "約定日") {
			// 種別、銘柄、売買を取得する
			$ct_category = ENV_STR_OFX_FUND;
			// 銘柄一覧より名前を取得する
			$ct_uniqueid = mb_convert_uniquename($line[2]);
			$ct_uniqueidtype = ENV_STR_OFX_CODE_HC;
			$ct_secname = "";
			// 銘柄一覧より検索する
			foreach($investments as $investment) {
				if($investment[0] == ENV_STR_OFX_FUND && strpos($investment[3], $ct_uniqueid) !== false) {
					// 持ち高がある場合
					$ct_uniqueid = mb_convert_uniqueid($investment[3]);
					$ct_secname = $investment[2];
					break;
				}
			}
			if($ct_secname == "") {
				// 持ち高がない場合
				$ct_uniqueid = mb_convert_uniqueid(mb_convert_uniquename($line[2]));
				$ct_secname = mb_convert_string($line[2]);
				
				if(strpos($seclist, $ct_uniqueid) === false) {
					// SECLIST
					$seclist .= "<!-- SOLD -->";
					$seclist .= "<" . $ct_category . "INFO><SECINFO>";
					$seclist .= "<SECID><UNIQUEID>" . $ct_uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $ct_uniqueidtype . "</UNIQUEIDTYPE></SECID>";
					$seclist .= "<SECNAME>" . $ct_secname . "</SECNAME>";
					$seclist .= "</SECINFO></" . $ct_category . "INFO>";
					$seclist .= "\r\n";
				}
			}
			// 単価を取得する
			$ct_unitprice = round((integer)$line[6] / 10000, 4);
			// 手数料を取得する
			// $ct_commission = round((integer)$line[5] * $ct_unitprice - (integer)$line[10], 4);
			$ct_commission = round((integer)$line[10], 4);
			// 税を取得する
			$ct_taxes = "";
			switch($line[4]) {
			case ENV_STR_ASSET_FUND_SELL:
				$ct_buytype = ENV_STR_OFX_SELL;
				$ct_units = -1;
				$ct_total = 1;
				break;
			case ENV_STR_ASSET_FUND_BUY:
			default:
				$ct_buytype = ENV_STR_OFX_BUY;
				$ct_units = 1;
				$ct_total = -1;
				break;
			}
			// 数量を取得する
			$ct_units *= (integer)$line[5];
			// 金額を取得する
			// マイナス値の手数料は許容されないため、金額に上乗せして調整する
			$ct_total *= ((integer)$line[9] + $ct_commission);
			// 約定日を取得する
			$ct_dttrade = parse_date($line[0]);
			// 通番を生成する
			if($ct_dttrade == $ct_date) $ct_num++; else $ct_num = 0;
			// トランザクション番号を生成する
			$ct_fitid = $ct_dttrade . "0000010" . sprintf("%05d", $ct_num);
			
			$ct = array();
			$ct["CATEGORY"] = $ct_category;
			$ct["FITID"] = $ct_fitid;
			$ct["DTTRADE"] = $ct_dttrade;
			$ct["UNIQUEID"] = $ct_uniqueid;
			$ct["SECNAME"] = $ct_secname;
			$ct["UNIQUEIDTYPE"] = $ct_uniqueidtype;
			$ct["UNITS"] = (string)$ct_units;
			$ct["UNITPRICE"] = (string)$ct_unitprice;
			// OFX仕様上、COMMISSION、FEES、TAXESはマイナス値を許容しない
			// $ct["COMMISSION"] = (string)abs($ct_commission);
			$ct["FEES"] = (string)abs($ct_fees);
			$ct["TAXES"] = (string)abs($ct_taxes);
			$ct["TOTAL"] = (string)$ct_total;
			$ct["SUBACCTSEC"] = ENV_STR_OFX_CASH;
			$ct["SUBACCTFUND"] = ENV_STR_OFX_CASH;
			$ct["BUYTYPE"] = $ct_buytype;
			array_push($cts, $ct);
			
			$ct_date = $ct_dttrade;
		}
	}
	
	$invtranlist = "";
	$invtranlist .= "<DTSTART>" . ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ . "</DTSTART>";
	$invtranlist .= "<DTEND>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTEND>";
	$invtranlist .= "\r\n";
	foreach($cts as $ct) {
		// INVTRANLIST
		$invtranlist .= "<" . $ct["BUYTYPE"] . $ct["CATEGORY"] . ">";
		$invtranlist .= "<INV" . $ct["BUYTYPE"] . ">";
		$invtranlist .= "<INVTRAN><FITID>" . $ct["FITID"] . "</FITID><DTTRADE>" . $ct["DTTRADE"] . ENV_STR_OFX_TZ . "</DTTRADE></INVTRAN>";
		$invtranlist .= "<SECID><UNIQUEID>" . $ct["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $ct["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$invtranlist .= "<UNITS>" . $ct["UNITS"] . "</UNITS>";
		$invtranlist .= "<UNITPRICE>" . $ct["UNITPRICE"] . "</UNITPRICE>";
		switch($ct["CATEGORY"]) {
		case ENV_STR_OFX_STOCK:
			$invtranlist .= "<TAXES>" . $ct["TAXES"] . "</TAXES>";
			$invtranlist .= "<FEES>" . $ct["FEES"] . "</FEES>";
			break;
		case ENV_STR_OFX_FUND:
			// マイナス値の手数料は許容されないため、金額に上乗せして調整する
			// $invtranlist .= "<COMMISSION>" . $ct["COMMISSION"] . "</COMMISSION>";
			break;
		case ENV_STR_OFX_CASH:
		default:
			break;
		}
		$invtranlist .= "<TOTAL>" . $ct["TOTAL"] . "</TOTAL>";
		$invtranlist .= "<SUBACCTSEC>" . $ct["SUBACCTSEC"] . "</SUBACCTSEC>";
		$invtranlist .= "<SUBACCTFUND>" . $ct["SUBACCTFUND"] . "</SUBACCTFUND>";
		$invtranlist .= "</INV" . $ct["BUYTYPE"] . ">";
		$invtranlist .= "<" . $ct["BUYTYPE"] . "TYPE>" . $ct["BUYTYPE"] . "</" . $ct["BUYTYPE"] . "TYPE>";
		$invtranlist .= "</" . $ct["BUYTYPE"] . $ct["CATEGORY"] . ">";
		$invtranlist .= "\r\n";
	}
	
	// 入出金照会CSVを取得する（入出金）
	$page = "/Light/Account/CashStatus/CashStatusCsv.asp";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
	$lines = array_reverse(parse_csv($body));
	$cd_date = "";
	$cd_num = 0;
	foreach($lines as $line) {
		if(count($line) == 4 && $line[0] != "[ 入出金確認 ]" && $line[0] != "お取り扱い日") {
			$cd = array();
			list($cd_name, $cd_memo, $cd_dummy) = explode(ENV_CHR_SEPARATOR, str_replace(array("　", "／"), array(ENV_CHR_SEPARATOR, ENV_CHR_SEPARATOR), $line[3]), 3);
			if($cd_memo != "") {
				$cd["NAME"] = mb_convert_string($cd_name);
				$cd["MEMO"] = mb_convert_string($cd_memo);
			} else {
				$cd["NAME"] = mb_convert_string($line[3]);
				$cd["MEMO"] = "";
			}
			if($cd["MEMO"] == "") $cd["MEMO"] = ENV_STR_OFX_MEMO;
			switch($cd_name) {
			case "振込入金":
			case "振込出金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
				break;
			case "配当金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIV;
				break;
			case "株式買付":
			case "株式売却":
			case "投信買付":
			case "投信売却":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CASH;
				break;
			case "譲渡益税還付金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DEP;
				break;
			case "確定配当調整金":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_INT;
				break;
			case "振込手数料":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_SRVCHG;
				break;
			case "自動引落":
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
				break;
			case "信用返済":
			case "品受":
			case "品渡":
			case "先物返済":
			case "OP返済":
			case "Cワラント売買":
			default:
				$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
				break;
			}
			// 約定日を取得する
			$cd_dtposted = parse_date($line[0]);
			// 通番を生成する
			if($cd_dtposted == $cd_date) $cd_num++; else $cd_num = 0;
			// トランザクション番号を生成する
			$cd["FITID"] = $cd_dtposted . "0000000" . sprintf("%05d", $cd_num);
			$cd["TRNAMT"] = (string)((double)parse_amount($line[1]) - (double)parse_amount($line[2]));
			$cd["DTPOSTED"] = $cd_dtposted;
			$cd["SUBACCTFUND"] = ENV_STR_OFX_CASH;
			
			// INVTRANLIST
			$invtranlist .= "<INVBANKTRAN>";
			$invtranlist .= "<STMTTRN>";
			$invtranlist .= "<TRNTYPE>" . $cd["TRNTYPE"] . "</TRNTYPE>";
			$invtranlist .= "<DTPOSTED>" . $cd["DTPOSTED"] . ENV_STR_OFX_TZ . "</DTPOSTED>";
			$invtranlist .= "<TRNAMT>" . $cd["TRNAMT"] . "</TRNAMT>";
			$invtranlist .= "<FITID>" . $cd["FITID"] . "</FITID>";
			$invtranlist .= "<NAME>" . $cd["NAME"] . "</NAME>";
			$invtranlist .= "<MEMO>" . $cd["MEMO"] . "</MEMO>";
			$invtranlist .= "</STMTTRN>";
			$invtranlist .= "<SUBACCTFUND>" . $cd["SUBACCTFUND"] . "</SUBACCTFUND>";
			$invtranlist .= "</INVBANKTRAN>";
			$invtranlist .= "\r\n";
			
			$cd_date = $cd_dtposted;
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$method = "POST";
	$protocol = "http";
	$host = "s20.si1.kabu.co.jp";
	$page = "/Members/LogOut.asp";
	$query = "";
	$cookie = kabucom_updatecookie($head, $cookie);
	list($head, $body) = kabucom_http11($method, $protocol, $host, $page, $query, $cookie);
	
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
	$invstmtmsgsrsv1 .= "<ACCTID>" . $acctid . $account . "</ACCTID>";
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
	$invstmtmsgsrsv1 .= "<AVAILCASH>0</AVAILCASH>";
	$invstmtmsgsrsv1 .= "<MARGINBALANCE>0</MARGINBALANCE>";
	$invstmtmsgsrsv1 .= "<SHORTBALANCE>0</SHORTBALANCE>";
	$invstmtmsgsrsv1 .= "</INVBAL>";
	$invstmtmsgsrsv1 .= "\r\n";
	$invstmtmsgsrsv1 .= "<MKTGINFO>" . mb_convert_string($settings["name"]) . "</MKTGINFO>";
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

// HTTP/1.1
function kabucom_http11($method, $protocol, $host, $page = "/", $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.?kabu\.co(?:m|\.jp)$/", $host) > 0) $ret = http11($method, $protocol, $host, 0, $page, $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function kabucom_updatecookie($head, $cookie) {
	$ret = "";
	$cookies = array();
	
	$ckvs = explode(";", $cookie);
	foreach($ckvs as $ckv) {
		list($ck, $cv) = explode("=", $ckv, 2);
		$ck = trim($ck);
		$cv = trim($cv);
		if($ck != "" && $cv != "") $cookies[$ck] = $ck . "=" . $cv;
	}
	
	$cks = array("ASPSESSIONID[^=]*", "MASESSIONID", "KUID", "KSITEID", "KTICKET", "MenuOption", "CookieOFF", "MSCSFirstRequestedURL", "UserID", "SsLogonChannel", "SsLogonHost", "RDN", "MASESSIONID");
	foreach($cks as $ck) if(preg_match_all("/[Ss][Ee][Tt]-[Cc][Oo][Oo][Kk][Ii][Ee][\s\t]*:[\s\t]*(" . $ck . ")=([^;\r\n]*)/", $head, $matches) > 0) $cookies[$matches[1][count($matches[1]) - 1]] = $matches[1][count($matches[1]) - 1] . "=" . $matches[2][count($matches[1]) - 1];
	
	$ret = implode("; ", $cookies);
	return $ret;
}

?>
