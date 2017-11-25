<?php
/*
nrkn.inc: 日本レコード・キーピング・ネットワークのHTMLよりOFXファイルを生成する
Copyright (C) 2012-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["userId"];
$pass = $ofxforms["password"];

$resp = array();

define("ENV_STR_ASSET_REINVESTMENT", "再投資");
define("ENV_STR_ASSET_CONTRIBUTION", "拠出");
define("ENV_STR_ASSET_SELL", "資産売却");
define("ENV_STR_ASSET_PAYMENT", "支払");

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する
$as = parse_tag($body, "a");
foreach($as as $a) if(strip_tags($a["innerHTML"]) == "ログイン") {
	$method = "GET";
	$uris = parse_uri($a["href"], $uris);
	$query = "";
	$cookie = nrkn_update_cookie($head, $cookie);
	list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
	break;
}

// ログインする
$forms = parse_tag($body, "form");
if(count($forms) >= 1) {
	$inputs = parse_tag($forms[0]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["userId"] = "userId=" . $user;
	$queries["password"] = "password=" . $pass;
	
	$head .= "Set-Cookie: cookiesCheck=true\r\n";
	
	$method = $forms[0]["method"];
	$uris = parse_uri($forms[0]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = nrkn_update_cookie($head, $cookie);
	list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
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
	$cookie = nrkn_update_cookie($head, $cookie);
	list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "システムメンテナンス中") !== false || strpos($body, "システムメンテナンスのため") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回ご利用（ログイン）日時") === false) {
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
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	$acctid = "";
	$acctname = "";
	
	// tdタグをパースする
	$f = false;
	$tds = parse_tag($body, "td", true); // 再帰的に取得する
	foreach($tds as $td) {
		if($f == true) {
			$acctid = trim(strip_tags($td["innerHTML"]));
			$f = false;
		}
		if($td["innerHTML"] == "加入者番号") $f = true;
	}
	
	if($acctid == "") $acctid = $user;
	
	// 資産評価額照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "W37S1040_Form") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// tdタグをパースする
	$tds = parse_tag($body, "td", true); // 再帰的に取得する
	$securities = array();
	$bufs = array();
	foreach($tds as $td) switch($td["class"]) {
	case "s1040-t2-o1":
	case "s1040-t2-e1":
		$bufs["SECNAME"] = trim(strip_tags($td["innerHTML"])); // 商品名
		break;
	case "s1040-t2-o2":
	case "s1040-t2-e2":
		$bufs["TYPE"] = trim(strip_tags($td["innerHTML"])); // 商品分類
		break;
	case "s1040-t2-o3":
	case "s1040-t2-e3":
		$bufs["UNITS"] = parse_amount($td["innerHTML"]); // 数量
		break;
	case "s1040-t2-o4":
	case "s1040-t2-e4":
		$bufs["UNITPRICE"] = parse_amount($td["innerHTML"]); // 基準価額
		break;
	case "s1040-t2-o5":
	case "s1040-t2-e5":
		$bufs["MKTVAL"] = parse_amount($td["innerHTML"]); // 評価金額
		break;
	case "s1040-t2-o7":
	case "s1040-t2-e7":
		$bufs["MEMO"] = parse_amount($td["innerHTML"]); // 損益
		break;
	case "s1040-t2-o9":
	case "s1040-t2-e9":
		$bufs["DTPRICEASOF"] = ($td["innerHTML"] == "&nbsp;"? ENV_STR_DATE_TODAY: parse_date($td["innerHTML"])); // 基準日
		
		if($bufs["TYPE"] == "現金") {
			$marginbalance = (string)((double)$marginbalance + (double)$bufs["MKTVAL"]);
			$availcash = $marginbalance;
		} else {
			$bufs["UNIQUEIDTYPE"] = $settings["code"];
			$bufs["HELDINACCT"] = ENV_STR_OFX_TRNTYPE_CASH; // 現金
			$bufs["POSTYPE"] = "LONG";
			$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
			$bufs["UNITPRICE"] = ($bufs["UNITPRICE"] == "0"? 0: (string)round((double)$bufs["UNITPRICE"] / round((double)$bufs["UNITS"] * (double)$bufs["UNITPRICE"] / (double)$bufs["MKTVAL"]), 4));
			array_push($securities, $bufs);
		}
		$bufs = array();
		break;
	default:
		break;
	}
	
	// 取引履歴照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "W37S1050_Head") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// リダイレクトする
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "W37S1050_Form") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$selects = parse_tag($form["innerHTML"], "select");
		foreach($selects as $select) {
			$options = parse_tag($select["innerHTML"], "option");
			foreach($options as $option) if($select["name"] != "" && $option["selected"] == "selected") $queries[$select["name"]] = urlencode($select["name"]) . "=" . urlencode($option["value"]);
		}
		$queries["lst_ddiv_yyyy_refBegin"] = "lst_ddiv_yyyy_refBegin=" . substr(ENV_STR_DATE_PASTDAY, 0, 4);
		$queries["lst_ddiv_mm_refBegin"] = "lst_ddiv_mm_refBegin=" . substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["lst_ddiv_yyyy_refEnd"] = "lst_ddiv_yyyy_refEnd=" . substr(ENV_STR_DATE_TODAY, 0, 4);
		$queries["lst_ddiv_mm_refEnd"] = "lst_ddiv_mm_refEnd=" . substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["chk_transType"] = "chk_transType=01&chk_transType=04&chk_transType=06&chk_transType=07"; // 拠出、収益分配・満期継続、資産売却、支払
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$body_old = $body;
	$banks = array();
	$exchanges = array();
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// 明細表示画面を取得する
	$forms = parse_tag($body, "form");
	$f = false;
	foreach($forms as $form) if($form["name"] == "W37S1060_Form") {
		if($f == false) {
			$f = true; // 1つ目のフォームは明細でないため無視する
			continue;
		}
		
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		
		// tdタグをパースする
		$tds = parse_tag($body, "td", true); // 再帰的に取得する
		$bufs = array();
		foreach($tds as $td) switch($td["class"]) {
		// 収益分配
		case "s1110-t1-d1":
			$bufs["DTPOSTED"] = parse_date($td["innerHTML"]); // 決算日・満期日
			break;
		case "s1110-t1-d2":
			$bufs["NAME"] = trim(strip_tags($td["innerHTML"])); // 取引区分
			break;
		case "s1110-t2-o1":
		case "s1110-t2-e1":
			$bufs["MEMO"] = trim(strip_tags($td["innerHTML"])); // 商品名
			break;
		case "s1110-t2-o5":
		case "s1110-t2-e5":
			$bufs["TRNAMT"] = parse_amount($td["innerHTML"]); // 分配金額・利息
			
			$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIV; // 配当金
			$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
			array_push($banks, $bufs);
			$bufs = array();
			break;
		
		// 収益分配（再投資）
		case "s1110-t3-o1":
		case "s1110-t3-e1":
			$bufs["SECNAME"] = trim(strip_tags($td["innerHTML"])); // 商品名
			break;
		case "s1110-t3-o8":
		case "s1110-t3-e8":
			$bufs["DTTRADE"] = parse_date($td["innerHTML"]); // 約定日
			break;
		case "s1110-t3-o3":
		case "s1110-t3-e3":
			$memo = trim(strip_tags($td["innerHTML"])); // 備考
			if($memo != "") $memo = ENV_CHR_CONCATENATOR . $memo;
			$bufs["MEMO"] = ENV_STR_ASSET_REINVESTMENT . $memo; // 再投資
			break;
		case "s1110-t3-o4":
		case "s1110-t3-e4":
			$bufs["UNITS"] = parse_amount($td["innerHTML"]); // 購入数量
			break;
		case "s1110-t3-o5":
		case "s1110-t3-e5":
			$bufs["UNITPRICE"] = parse_amount($td["innerHTML"]); // 購入単価
			break;
		case "s1110-t3-o6":
		case "s1110-t3-e6":
			$bufs["TOTAL"] = (string)(-1 * (double)parse_amount($td["innerHTML"])); // 購入金額
			break;
		case "s1110-t3-o7":
		case "s1110-t3-e7":
			if(trim(strip_tags($td["innerHTML"])) == "約定済") {
				$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // すべてを投資信託と見なす
				$bufs["SUBACCTSEC"] = ENV_STR_OFX_CASH; // 現金
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				$bufs["BUYTYPE"] = ENV_STR_OFX_BUY; // 買付
				if((double)$bufs["UNITS"] != 0 && (double)$bufs["MKTVAL"] != 0) {
					$bufs["UNITPRICE"] = (string)round((double)$bufs["UNITPRICE"] / round((double)$bufs["UNITS"] * (double)$bufs["UNITPRICE"] / (double)$bufs["MKTVAL"]), 4);
					array_push($exchanges, $bufs);
				}
			}
			$bufs = array();
			break;
		
		// 拠出（現金）
		case "s1070-t1-d2":
			$cd_date = parse_date($td["innerHTML"]); // 拠出日
			break;
		case "s1070-t1-h3":
			if($td["innerHTML"] != "拠出日" && strpos($td["innerHTML"], "＋") === false) $bufs["NAME"] = str_replace(array("（１）", "（２）", "額"), array("", "", ""), trim(strip_tags($td["innerHTML"]))); // 拠出内訳
			break;
		case "s1070-t1-d3":
			if($bufs["NAME"] != "") {
				$bufs["TRNAMT"] = parse_amount($td["innerHTML"]); // 掛金額
				
				$bufs["DTPOSTED"] = $cd_date;
				$bufs["MEMO"] = ENV_STR_ASSET_CONTRIBUTION;
				$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				array_push($banks, $bufs);
				$bufs = array();
			}
			break;
		
		// 拠出（投資）
		case "s1070-t2-o1":
		case "s1070-t2-e1":
			$bufs["SECNAME"] = trim(strip_tags($td["innerHTML"])); // 商品名
			break;
		case "s1070-t2-o8":
		case "s1070-t2-e8":
			$bufs["DTTRADE"] = parse_date($td["innerHTML"]); // 約定日
			break;
		case "s1070-t2-o3":
		case "s1070-t2-e3":
			$memo = trim(strip_tags($td["innerHTML"])); // 備考
			if($memo != "") $memo = ENV_CHR_CONCATENATOR . $memo;
			$bufs["MEMO"] = ENV_STR_ASSET_CONTRIBUTION . $memo; // 拠出
			break;
		case "s1070-t2-o4":
		case "s1070-t2-e4":
			$bufs["TOTAL"] = (string)(-1 * (double)parse_amount($td["innerHTML"])); // 購入金額
			break;
		case "s1070-t2-o5":
		case "s1070-t2-e5":
			$bufs["UNITPRICE"] = parse_amount($td["innerHTML"]); // 購入単価
			break;
		case "s1070-t2-o6":
		case "s1070-t2-e6":
			$bufs["UNITS"] = parse_amount($td["innerHTML"]); // 購入数量
			break;
			
		case "s1070-t2-o7":
		case "s1070-t2-e7":
			if(trim(strip_tags($td["innerHTML"])) == "約定済") {
				$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
				$bufs["SUBACCTSEC"] = ENV_STR_OFX_CASH; // 現金
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				$bufs["UNIQUEIDTYPE"] = $settings["code"];
				$bufs["BUYTYPE"] = ENV_STR_OFX_BUY; // 買付
				$bufs["UNITPRICE"] = (string)round((double)$bufs["UNITPRICE"] / round((double)$bufs["UNITS"] * (double)$bufs["UNITPRICE"] / (double)$bufs["TOTAL"] * -1), 4);
				array_push($exchanges, $bufs);
			}
			$bufs = array();
			break;
		
		// 資産売却（投資）
		case "s1080-t2-o1":
		case "s1080-t2-e1":
			$bufs["SECNAME"] = trim(strip_tags($td["innerHTML"])); // 商品名
			break;
		case "s1080-t2-o8":
		case "s1080-t2-e8":
			$bufs["DTTRADE"] = parse_date($td["innerHTML"]); // 約定日
			break;
		case "s1080-t2-o3":
		case "s1080-t2-e3":
			$memo = trim(strip_tags($td["innerHTML"])); // 備考
			$bufs["MEMO"] = ENV_STR_ASSET_SELL; // 資産売却
			break;
		case "s1080-t2-o6":
		case "s1080-t2-e6":
			$bufs["TOTAL"] = (string)((double)parse_amount($td["innerHTML"])); // 売却金額
			break;
		case "s1080-t2-o5":
		case "s1080-t2-e5":
			$bufs["UNITPRICE"] = parse_amount($td["innerHTML"]); // 売却単価
			break;
		case "s1080-t2-o4":
		case "s1080-t2-e4":
			$bufs["UNITS"] = (string)(-1 * (double)parse_amount($td["innerHTML"])); // 売却数量
			break;
			
		case "s1080-t2-o7":
		case "s1080-t2-e7":
			if(trim(strip_tags($td["innerHTML"])) == "約定済") {
				$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
				$bufs["SUBACCTSEC"] = ENV_STR_OFX_CASH; // 現金
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				$bufs["UNIQUEIDTYPE"] = $settings["code"];
				$bufs["BUYTYPE"] = ENV_STR_OFX_SELL; // 売付
				$bufs["UNITPRICE"] = (string)round((double)$bufs["UNITPRICE"] / round((double)$bufs["UNITS"] * (double)$bufs["UNITPRICE"] / (double)$bufs["TOTAL"] * -1), 4);
				array_push($exchanges, $bufs);
				array_push($securities, $bufs);
			}
			$bufs = array();
			break;
		
		// 支払（現金）
		case "s1820-t2-d2":
			$bufs["NAME"] = $td["innerHTML"]; // 取引事由
			break;
		case "s1820-t3-d2":
			if($bufs["NAME"] != "") {
				$bufs["TRNAMT"] = "-" . parse_amount($td["innerHTML"]); // 支払額
				
				$bufs["MEMO"] = ENV_STR_ASSET_PAYMENT;
				$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_OTHER;
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
			}
			break;
		case "s1820-t4-d2":
			$cd_date = parse_date(trim($td["innerHTML"])); // 支払予定日
			if($bufs["NAME"] != "") {
				$bufs["DTPOSTED"] = $cd_date;
				array_push($banks, $bufs);
				$bufs = array();
			}
			break;
			
		default:
			break;
		}
		
		$body = $body_old;
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// プラン情報照会画面を取得する
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "W37S1130_Head") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		break;
	}
	
	$investments = array();
	$prefix = "";
	
	// tdタグをパースする
	$tds = parse_tag($body, "td", true); // 再帰的に取得する
	foreach($tds as $td) switch($td["class"]) {
	case "s0000-t3-d1":
		if($acctname == "") $acctname = ENV_CHR_CONCATENATOR . trim(strip_tags($td["innerHTML"])); // プラン名
		break;
	case "s0000-t3-d2":
		if($prefix == "") $prefix = trim(strip_tags($td["innerHTML"])); // プラン番号
		break;
	case "s1130-t3-o1":
	case "s1130-t3-e1":
		$uniqueid = $prefix . trim(strip_tags($td["innerHTML"])); // 商品コード
		break;
	case "s1130-t3-o2":
	case "s1130-t3-e2":
		$secname = trim(strip_tags($td["innerHTML"])); // 商品名
		$investments[$uniqueid] = $secname;
		break;
	
	default:
		break;
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$forms = parse_tag($body, "form");
	foreach($forms as $form) if($form["name"] == "W37S0020_Head") {
		$inputs = parse_tag($form["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		
		$method = $form["method"];
		$uris = parse_uri($form["action"], $uris);
		$query = implode("&", $queries);
		$cookie = nrkn_update_cookie($head, $cookie);
		list($head, $body) = nrkn_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// INVTRANLIST（投資）
	$invtranlist = "";
	$invtranlist .= "<DTSTART>" . ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ . "</DTSTART>";
	$invtranlist .= "<DTEND>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTEND>";
	$invtranlist .= "\r\n";
	
	$cd_date = "";
	$cd_num = 0;
	
	$usort_exchanges = function($a, $b) {
		strcmp($a["DTTRADE"], $b["DTTRADE"]);
	};
	usort($exchanges, $usort_exchanges);
	
	foreach($exchanges as $exchange) {
		$uniqueid = array_search($exchange["SECNAME"], $investments);
		if($uniqueid === false) $uniqueid = mb_convert_uniqueid($exchange["SECNAME"]);
		
		if($exchange["DTTRADE"] == $cd_date) $cd_num++; else $cd_num = 0;
		$fitid = $exchange["DTTRADE"] . "0000001" . sprintf("%05d", $cd_num);
		
		$invtranlist .= "<" . $exchange["BUYTYPE"] . $exchange["CATEGORY"] . ">";
		$invtranlist .= "<INV" . $exchange["BUYTYPE"] . ">";
		$invtranlist .= "<INVTRAN>";
		$invtranlist .= "<FITID>" . $fitid . "</FITID>";
		$invtranlist .= "<DTTRADE>" . $exchange["DTTRADE"] . ENV_STR_OFX_TZ . "</DTTRADE>";
		$invtranlist .= "</INVTRAN>";
		$invtranlist .= "<SECID><UNIQUEID>" . $uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $exchange["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$invtranlist .= "<UNITS>" . $exchange["UNITS"] . "</UNITS>";
		$invtranlist .= "<UNITPRICE>" . $exchange["UNITPRICE"] . "</UNITPRICE>";
		$invtranlist .= "<TOTAL>" . $exchange["TOTAL"] . "</TOTAL>";
		$invtranlist .= "<SUBACCTSEC>" . $exchange["SUBACCTSEC"] . "</SUBACCTSEC>";
		$invtranlist .= "<SUBACCTFUND>" . $exchange["SUBACCTFUND"] . "</SUBACCTFUND>";
		$invtranlist .= "</INV" . $exchange["BUYTYPE"] . ">";
		$invtranlist .= "<" . $exchange["BUYTYPE"] . "TYPE>" . $exchange["BUYTYPE"] . "</" . $exchange["BUYTYPE"] . "TYPE>";
		$invtranlist .= "</" . $exchange["BUYTYPE"] . $exchange["CATEGORY"] . ">";
		$invtranlist .= "\r\n";
		
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
		$uniqueid = array_search($security["SECNAME"], $investments);
		if($uniqueid === false) $uniqueid = mb_convert_uniqueid($security["SECNAME"]);
		
		// SECLIST
		$seclist .= "<" . $security["CATEGORY"] . "INFO><SECINFO>";
		$seclist .= "<SECID><UNIQUEID>" . $uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$seclist .= "<SECNAME>" . $security["SECNAME"] . "</SECNAME>";
		$seclist .= "</SECINFO></" . $security["CATEGORY"] . "INFO>";
		$seclist .= "\r\n";
		
		if($security["MEMO"] == ENV_STR_ASSET_SELL) continue;
		
		$invposlist .= "<POS" . $security["CATEGORY"] . "><INVPOS>";
		$invposlist .= "<SECID><UNIQUEID>" . $uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
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

function nrkn_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.nrkn\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie, true, ENV_PRODUCT_UA . " Windows NT"); // ログイン時にUser-Agentをチェックされる
	return explode("\r\n\r\n", $ret, 2);
}

function nrkn_update_cookie($head, $cookie) {
	return update_cookie(array("cookiesCheck", "JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

?>
