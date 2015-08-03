<?php
/*
common.inc: 定数・共通関数を定義する
Copyright (C) 2008-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU All-Permissive License and Beatrek Origin License.
*/

define("ENV_PRODUCT_CODENAME", "PURSE");
define("ENV_PRODUCT_VERSION", "1.1");
define("ENV_PRODUCT_ID", ENV_PRODUCT_CODENAME . "/" . ENV_PRODUCT_VERSION);
define("ENV_PRODUCT_FAMILY", "MoneySound");
define("ENV_PRODUCT_COMMENT", "+https://github.com/outerguy/moneysound");
define("ENV_PRODUCT_UA", "Mozilla/5.0 (compatible; " . ENV_PRODUCT_ID . (defined("ENV_PRODUCT_FAMILY") == true && ENV_PRODUCT_FAMILY != ""? "; " . ENV_PRODUCT_FAMILY: "") . (defined("ENV_PRODUCT_COMMENT") == true && ENV_PRODUCT_COMMENT != ""? "; " . ENV_PRODUCT_COMMENT: "") . ")");

define("ENV_BOOL_DEBUG", false); // 開発者向け（デバッグ）機能を有効とするか否か

define("ENV_FILE_DIR_COMMON", "./common/");

define("ENV_FILE_TEMPLATE_FI", "fi.xml");
define("ENV_FILE_TEMPLATE_OFX", "ofx.xml");

define("ENV_NUM_STATUS_SUCCESS", 200);
define("ENV_NUM_STATUS_ADDITION", 202);
define("ENV_NUM_STATUS_NONE", 204);
define("ENV_NUM_STATUS_FAILURE", 403);
define("ENV_NUM_STATUS_MAINTENANCE", 503);
define("ENV_NUM_STATUS_CAUTION", 511);

// XMLファイルに定義されている金融機関の情報を条件に応じて取り出す
function get_fi_settings($fiid = "") {
	$ret = array();
	$xml = simplexml_load_file(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_FI);
	$settings = array();
	foreach($xml->fi as $fi) {
		$id = mb_htmlspecialchars((string)$fi["id"]);
		$settings[$id]["fiid"] = mb_htmlspecialchars((string)$id);
		$settings[$id]["type"] = mb_htmlspecialchars((string)$fi->type);
		$settings[$id]["name"] = mb_htmlspecialchars((string)$fi->name);
		$settings[$id]["home"] = mb_htmlspecialchars((string)$fi->home);
		if(isset($fi["code"]) == true) $settings[$id]["code"] = mb_htmlspecialchars((string)$fi["code"]);
		if(isset($fi->additional) == true) $settings[$id]["additional"] = mb_htmlspecialchars(implode("|", array((string)$fi->additional, (string)$fi->additional["input"], (string)$fi->additional["type"])));
		if(isset($fi->help) == true) $settings[$id]["help"] = preg_replace("/^<help>[\r\n]*([\w\W]*?)<\/help>[\r\n]*$/", "\\1", $fi->help->asXML());
		
		$inputs = array();
		foreach($fi->form->input as $input) {
			array_push($inputs, mb_htmlspecialchars((string)$input["id"]));
			$settings[$id][(string)$input["id"]] = mb_htmlspecialchars(implode("|", array((string)$input, (string)$input["type"])));
		}
		$settings[$id]["form"] = implode("|", $inputs);
		
		if(isset($fi->itemlist) == true) foreach($fi->itemlist->item as $item) $settings[$id][(string)$item["id"]] = mb_htmlspecialchars((string)$item);
	}
	
	$ret = ($fiid != "" && isset($settings[$fiid]) == true? $settings[$fiid]: $settings);
	
	return $ret;
}

function mb_htmlspecialchars($str) {
	return htmlspecialchars($str, ENT_COMPAT | ENT_XML1, "UTF-8");
}

// $codeに合致するHTTP/1.1ステータスコードの文字列を取得する
function get_http_status($code) {
	$status = "Unknown Status-code";
	switch((integer)$code) {
	// 100
	case 100:
		$status = "Continue";
		break;
	case 101:
		$status = "Switching Protocols";
		break;
	case 102:
		$status = "Processing";
		break;
	// 200
	case 200:
		$status = "OK";
		break;
	case 201:
		$status = "Created";
		break;
	case 202:
		$status = "Accepted";
		break;
	case 203:
		$status = "Non-Authoritative Information";
		break;
	case 204:
		$status = "No Content";
		break;
	case 205:
		$status = "Reset Content";
		break;
	case 206:
		$status = "Partial Content";
		break;
	case 207:
		$status = "Multi-Status";
		break;
	case 208:
		$status = "Already Reported";
		break;
	case 226:
		$status = "IM Used";
		break;
	// 300
	case 300:
		$status = "Multiple Choices";
		break;
	case 301:
		$status = "Moved Parmanently";
		break;
	case 302:
		$status = "Found";
		break;
	case 303:
		$status = "See Other";
		break;
	case 304:
		$status = "Not Modified";
		break;
	case 305:
		$status = "Use Proxy";
		break;
	case 306:
		$status = "(Unused)";
		break;
	case 307:
		$status = "Temporary Redirect";
		break;
	case 308:
		$status = "Permanent Redirect";
		break;
	// 400
	case 400:
		$status = "Bad Request";
		break;
	case 401:
		$status = "Unautorized";
		break;
	case 402:
		$status = "Payment Required";
		break;
	case 403:
		$status = "Forbidden";
		break;
	case 404:
		$status = "Not Found";
		break;
	case 405:
		$status = "Method Not Allowed";
		break;
	case 406:
		$status = "Not Acceptable";
		break;
	case 407:
		$status = "Proxy Authentication Required";
		break;
	case 408:
		$status = "Request Timeout";
		break;
	case 409:
		$status = "Conflict";
		break;
	case 410:
		$status = "Gone";
		break;
	case 411:
		$status = "Length Required";
		break;
	case 412:
		$status = "Precondition Failed";
		break;
	case 413:
		$status = "Request Entity Too Large";
		break;
	case 414:
		$status = "Request-URI Too Long";
		break;
	case 415:
		$status = "Unsupported Media Type";
		break;
	case 416:
		$status = "Requested Range Not Satisfiable";
		break;
	case 417:
		$status = "Expectation Failed";
		break;
	case 422:
		$status = "Unprocessable Entity";
		break;
	case 423:
		$status = "Locked";
		break;
	case 424:
		$status = "Failed Dependency";
		break;
	case 426:
		$status = "Upgrade Required";
		break;
	case 428:
		$status = "Precondition Required";
		break;
	case 429:
		$status = "Too Many Requests";
		break;
	case 431:
		$status = "Request Header Fields Too Large";
		break;
	// 500
	case 500:
		$status = "Internal Server Error";
		break;
	case 501:
		$status = "Not Implemented";
		break;
	case 502:
		$status = "Bad Gateway";
		break;
	case 503:
		$status = "Service Unavailable";
		break;
	case 504:
		$status = "Gateway Timeout";
		break;
	case 505:
		$status = "HTTP Version Not Supported";
		break;
	case 506:
		$status = "Variant Also Negotiates";
		break;
	case 507:
		$status = "Insufficient Storage";
		break;
	case 508:
		$status = "Loop Detected";
		break;
	case 510:
		$status = "Not Extended";
		break;
	case 511:
		$status = "Network Authentication Required";
		break;
	default:
		// 何もしない
		break;
	}
	return "HTTP/1.1 " . (string)$code . " " . $status;
}

?>
