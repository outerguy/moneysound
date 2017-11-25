<?php
/*
dccard.inc: DCカードのCSVよりOFXファイルを変換・結合する
Copyright (C) 2012-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["webId"];
$pass = $ofxforms["webPassword"];
$birth = $ofxforms["webBirthDay"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する（NEWS+PLUS）
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = mun_http11($method, $uris, $query, $cookie);

// ログイン画面を取得する（NEWS+PLUS）
$as = parse_tag($body, "a");
$c = parse_tag_search($as, "innerHTML", "ログインする");
if($c != -1) {
	$method = "GET";
	$uris = parse_uri($as[$c]["href"], $uris);
	$cookie = mun_update_cookie($head, $cookie);
	list($head, $body) = mun_http11($method, $uris, $query, $cookie);
}

// カードブランドを選択する
$as = parse_tag($body, "a");
foreach($as as $a) {
	$imgs = parse_tag($a["innerHTML"], "img");
	if(parse_tag_search($imgs, "alt", "DCカード") != -1) {
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$cookie = mun_update_cookie($head, $cookie);
		list($head, $body) = mun_http11($method, $uris, $query, $cookie);
		break;
	}
}

// ログインする（NEWS+PLUS）
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "id", "loginFormNavi");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	$queries["webId"] = "webId=" . $user;
	$queries["webPassword"] = "webPassword=" . $pass;
	$queries["webAddAuthType"] = "webAddAuthType=2";
	$queries["webBirthDay"] = "webBirthDay=" . $birth;
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = mun_update_cookie($head, $cookie);
	list($head, $body) = mun_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = mun_update_cookie($head, $cookie);
		list($head, $body) = mun_http11($method, $uris, $query, $cookie);
	}
}

$as = parse_tag($body, "a");
foreach($as as $a) {
	switch(trim(strip_tags($a["innerHTML"]))) {
	case "ログアウト":
		// ログアウト画面を退避する（NEWS+PLUS）
		$method_mun = "GET";
		$uris_mun = parse_uri($a["href"], $uris);
		$query_mun = "";
		$cookie_mun = $cookie;
		break;
	case "WEBサービストップ":
		// WEBサービストップ画面を取得する（NEWS+PLUS）
		$method = "GET";
		$uris = parse_uri($a["href"], $uris);
		$query = "";
		$cookie = mun_update_cookie($head, $cookie);
		list($head, $body) = mun_http11($method, $uris, $query, $cookie);
		break;
	default:
		break;
	}
}

// 以降は各カードブランドの画面に遷移する
$cookie = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "id", "seamlessForm");
if($c != -1) {
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
	
	$method = $forms[$c]["method"];
	$uris = parse_uri($forms[$c]["action"], $uris);
	$query = implode("&", $queries);
	$cookie = dccard_update_cookie($head, $cookie);
	list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
}

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = dccard_update_cookie($head, $cookie);
		list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
	}
}

// MenuFrame画面を取得する
$frames = parse_tag($body, "frame");
$c = parse_tag_search($frames, "name", "MenuFrame");
if($c != -1) {
	$method = "GET";
	$uris = parse_uri($frames[$c]["src"], $uris);
	$query = "";
	$cookie = dccard_update_cookie($head, $cookie);
	list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
}

if(strpos($body, "現在サービス停止中") !== false || strpos($body, "メンテナンス作業のためサービスを一時停止") !== false || strpos($body, "システムメンテナンスのため") !== false) {
	// システムメンテナンス画面の場合
	$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
	$resp["method"] = $method;
	$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
	$resp["query"] = $query;
	$resp["cookie"] = $cookie;
	$resp["head"] = $head;
	$resp["body"] = $body;
	$resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "前回ログイン時間") === false) {
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
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	
	$as = parse_tag($body, "a");
	foreach($as as $a) {
		$imgs = parse_tag($a["innerHTML"], "img");
		
		// ログアウト画面を退避する
		if(parse_tag_search($imgs, "alt", "ログアウト") != -1) $uris_old = parse_uri($a["href"], $uris);
		
		// ご利用代金明細照会（確定）画面を取得する
		if(parse_tag_search($imgs, "alt", "ご利用代金明細照会（確定）") != -1) {
			$method = "GET";
			$uris = parse_uri($a["href"], $uris);
			$query = "";
			$cookie = dccard_update_cookie($head, $cookie);
			list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
		}
	}
	
	// MainFrame画面を取得する
	$frames = parse_tag($body, "frame");
	$c = parse_tag_search($frames, "name", "MainFrame");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($frames[$c]["src"], $uris);
		$query = "";
		$cookie = dccard_update_cookie($head, $cookie);
		list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
	}
	
	// PBodyFrame画面を取得する
	$frames = parse_tag($body, "frame");
	$c = parse_tag_search($frames, "name", "PBodyFrame");
	if($c != -1) {
		$method = "GET";
		$uris = parse_uri($frames[$c]["src"], $uris);
		$query = "";
		$cookie = dccard_update_cookie($head, $cookie);
		list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$scripts = parse_tag($body, "script");
	foreach($scripts as $script) if(preg_match("/location\.href[\s\t]*=[\s\t]*\"(.*?)\";/i", $script["innerHTML"], $matches) > 0) {
		$method = "GET";
		$uris = parse_uri($matches[1], $uris);
		$query = "";
		$cookie = dccard_update_cookie($head, $cookie);
		list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
		break;
	}
	
	// リダイレクトする（システムメンテナンス時）
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = dccard_update_cookie($head, $cookie);
			list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
		}
	}
	
	// システムメンテナンス画面の場合
	if(strpos($body, "お客様のご請求額を確定") !== false || strpos($body, "ただいまサーバーメンテナンス中") !== false || strpos($body, "ページを表示することができません") !== false) {
		$resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
		$resp["method"] = $method;
		$resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
		$resp["query"] = $query;
		$resp["cookie"] = $cookie;
		$resp["head"] = $head;
		$resp["body"] = $body;
		$resp["ofx"] = generate_ofx($resp["status"]);
	} else {
		// 最新の請求月を取得する（お支払日は10日であると仮定する）
		$selects = parse_tag($body, "select");
		$c = parse_tag_search($selects, "name", "selmonth");
		$selmonth = "";
		if($c != -1) {
			$options = parse_tag($selects[$c]["innerHTML"], "option");
			if(count($options) > 0) {
				$selmonth = $options[0]["value"];
				$m = sprintf("%02d", (integer)str_replace("月", "", trim(strip_tags($options[0]["innerHTML"]))));
				$ymd = (date("m") <= $m? date("Y"): (string)((integer)date("Y") + 1)) . $m . "10";
			}
		}
		
		$accounts = array();
		
		// カード数を取得する
		$c = 0;
		$as = parse_tag($body, "a");
		$bufs = array();
		foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "照会") != -1) array_push($bufs, $a["href"]);
		for($i = 0; $i < count($bufs); $i++) if(preg_match("/javascript:[^\']+?\'([0-9]+)\'.*?/i", $bufs[$i], $matches) > 0) {
			$accounts[$c]["id"] = (integer)$matches[1];
			$accounts[$c]["sendmonth"] = $selmonth;
			$accounts[$c]["acctid"] = "";
			$accounts[$c]["paydate"] = $ymd;
			$c++;
		}
		
		$body_old = $body;
		
		// カード数分ループする
		foreach($accounts as $account) {
			$body = $body_old;
			
			// 実行時間（タイムアウト）を再設定する
			@set_time_limit(ENV_NUM_TIMEOUT);
			
			// カード・照会月を選択する
			$scripts = parse_tag($body, "script");
			foreach($scripts as $script) if(preg_match("/location\.href[\s\t]*=[\s\t]*([^;]+?);/i", $script["innerHTML"], $matches) > 0) {
				$method = "GET";
				$uris = parse_uri(str_replace(array("'", "\"", " ", "+", "num", "sendmonth"), array("", "", "", "", (string)$account["id"], $account["sendmonth"]), $matches[1]), $uris);
				$query = "";
				$cookie = dccard_update_cookie($head, $cookie);
				list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
				break;
			}
			
			// RRFrame画面を取得する
			$frames = parse_tag($body, "frame");
			$c = parse_tag_search($frames, "name", "RRFrame");
			if($c != -1) {
				$method = "GET";
				$uris = parse_uri($frames[$c]["src"], $uris);
				$query = "";
				$cookie = dccard_update_cookie($head, $cookie);
				list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
			}
			
			// リダイレクトする
			$scripts = parse_tag($body, "script");
			foreach($scripts as $script) if(preg_match("/location\.href[\s\t]*=[\s\t]*\"(.*?)\";/i", $script["innerHTML"], $matches) > 0) {
				$method = "GET";
				$uris = parse_uri($matches[1], $uris);
				$query = "";
				$cookie = dccard_update_cookie($head, $cookie);
				list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
				break;
			}
			
			$trs = parse_tag($body, "tr", true); // 再帰的に取得する
			$bufs = array();
			foreach($trs as $tr) {
				$tds = parse_tag($tr["innerHTML"], "td");
				if(count($tds) == 2) {
					$k = trim(strip_tags($tds[0]["innerHTML"]));
					$v = trim(strip_tags($tds[1]["innerHTML"]));
					switch($k) {
					case "カード名称":
						// カード名称を取得する
						$account["acctname"] = implode(ENV_CHR_CONCATENATOR, array($settings["name"], $v));
						$account["name"] = $settings["name"];
						break;
					case "カード番号":
						// カード番号を取得する
						if(preg_match("/([0-9\*\-]+)/", str_replace("－", "-", $v), $matches) > 0) $account["acctid"] = $matches[1];
						break;
					case "お支払日":
						// お支払日を取得する
						$account["paydate"] = parse_date($v);
						break;
					case "今回ご請求合計額":
						// 今回ご請求合計額を取得する
						$account["ledge_balamt"] = parse_amount($v);
						break;
					default:
						break;
					}
				}
			}
			
			$str = "";
			
			// CSVファイルをダウンロードする
			$as = parse_tag($body, "a");
			$c = parse_tag_search($as, "innerHTML", "ダウンロード：テキスト（CSV）形式");
			if($c != -1) {
				$method = "GET";
				$uris = parse_uri($as[$c]["href"], $uris);
				$query = "";
				$cookie = dccard_update_cookie($head, $cookie);
				list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
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
			$creditcardmsgsrsv1 .= dccard_parse_csv($body, $account);
			$creditcardmsgsrsv1 .= "</CCSTMTRS>";
			$creditcardmsgsrsv1 .= "\r\n";
			$creditcardmsgsrsv1 .= "</CCSTMTTRNRS>";
			$creditcardmsgsrsv1 .= "\r\n";
			
			$creditcardmsgsrsv1 .= "</CREDITCARDMSGSRSV1>";
			$creditcardmsgsrsv1 .= "\r\n";
		}
		
		// OFXファイルを出力する
		if(strpos($creditcardmsgsrsv1, "<BANKTRANLIST>") === false) {
			// 明細が存在しない場合
			$resp["ofx"] = generate_ofx($resp["status"]);
		} else {
			// 明細が存在する場合
			$resp["ofx"] = generate_ofx($resp["status"], $creditcardmsgsrsv1);
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウトする
	$method = "GET";
	$uris = $uris_old;
	$query = "";
	$cookie = dccard_update_cookie($head, $cookie);
	list($head, $body) = dccard_http11($method, $uris, $query, $cookie);
	
	// ログアウトする（NEWS+PLUS）
	$method = $method_mun;
	$uris = $uris_mun;
	$query = $query_mun;
	$cookie = $cookie_mun;
	list($head, $body) = mun_http11($method, $uris, $query, $cookie);
}
return $resp;

// HTTP/1.1（NEWS+PLUS）
function mun_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.cr\.mufg\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function mun_update_cookie($head, $cookie) {
	return update_cookie(array("PHPSESSID", "session-srv", "m_cardBrand"), parse_header($head, "set-cookie"), $cookie);
}

// HTTP/1.1
function dccard_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.dccard\.co\.jp$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function dccard_update_cookie($head, $cookie) {
	return update_cookie(array("club", "ASPSESSIONID[A-Z]{8}"), parse_header($head, "set-cookie"), $cookie);
}

function dccard_parse_csv($str, $account) {
	$ret = "";
	$lines = parse_csv(mb_convert_string($str));
	$cds = array();
	$cds_balamt = $account["ledge_balamt"];
	$cds_paydate = $account["paydate"];
	$cds_s = "";
	$cds_e = "";
	$cd_date = "";
	$cd_num = 0;
	$ledge_balamt = $cds_balamt;
	
	foreach($lines as $line) {
		$cd = array();
		
		if(count($line) == 11 && $line[0] != "利用日") {
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
			$cd["NAME"] = $line[3];
			
			// 金額を取得する
			$cd["TRNAMT"] = (string)(-1 * (double)parse_amount($line[5]));
			$ledge_balamt += (double)$cd["TRNAMT"];
			
			// ご利用区分、および備考を取得する
			$cd["MEMO"] = ($line[2] != "" || $line[10] != ""? $line[2] . $line[10]: implode(ENV_CHR_CONCATENATOR, array($line[2], $line[10])));
			if($cd["MEMO"] == "") $cd["MEMO"] = ENV_STR_OFX_MEMO;
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
