<?php
/*
jaccscard.inc: ジャックスカードのHTMLよりOFXファイルを生成する
Copyright (C) 2012-2014 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Copyright (C) 2015 OFFICE OUTERGUY. All Rights Reserved.
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
$urls = parse_url($settings["home"]);
$method = "GET";
$protocol = ($urls["scheme"] != ""? $urls["scheme"]: "");
$host = ($urls["host"] != ""? $urls["host"]: "");
$page = ($urls["path"] != ""? $urls["path"]: "/");
$query = "";
$cookie = "";
list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);

// ログイン画面を取得する
$page = '/icmclub/icm_login.html';
list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);

$protocol = 'https';
$page = '/Service';
$query = '_TRANID=JALG00001_00M';
list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);

// ログインする
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "MainForm");
if($c != -1) {
	$method = "POST";
	$urls = parse_url($forms[$c]['action']);
	if($urls["scheme"] != "") $protocol = $urls["scheme"];
	if($urls["host"] != "") $host = $urls["host"];
	if($urls["path"] != "") $page = $urls["path"];
	if($urls["query"] != "") $page .= "?" . $urls["query"];
	
	$queries = jaccscard_getqueries($body);
	$queries["IUSERID"] .= $user;
	$queries["IPASSWORD"] .= $pass;
	$queries["_TRANID"] .= "JALG00001_01M";
	if(isset($queries["JALG00001_01M"]) == true) unset($queries["JALG00001_01M"]);
	$query = implode("&", $queries);
	$cookie = jaccscard_updatecookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
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
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ご利用状況の確認画面を取得する
	$queries = jaccscard_getqueries($body);
	$queries['_TRANID'] .= 'JAST00001_00M';
	$query = implode("&", $queries);
	$method = "POST";
	$cookie = jaccscard_updatecookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// カードが複数枚存在する場合、最初のカードを選択する
	// 当月を取得する
	$as = parse_tag($body, "a");
	$bufs = array();
	foreach($as as $a) if(parse_tag_search(parse_tag($a["innerHTML"], "img"), "alt", "ご利用代金明細を見る") != -1) array_push($bufs, $a["href"]);
	for($i = 0; $i < count($bufs); $i++) if(preg_match("/JavaScript:[^\']+?\'(JAST[0-9]{5}_[0-9]{2}[A-Z])\', \'([0-9]+)\'.*?/i", $bufs[$i], $matches) > 0) {
		$queries = jaccscard_getqueries($body);
		$queries['_TRANID'] .= $matches[1];
		$queries['_SUBINDEX'] .= $matches[2];
		$query = implode("&", $queries);
		$method = "POST";
		$cookie = jaccscard_updatecookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
		break;
	}
	
	$bodies[] = $body;
	
	// お支払日を取得する
	$paydate = "";
	$trs = parse_tag($body, "tr", true);
	$c = parse_tag_search($trs, "class", "first");
	if($c != -1) {
		if(preg_match("/<td>(.*?)<\/td>/s", $trs[$c]["innerHTML"], $matches) > 0) $paydate = parse_date(trim(str_replace("\t", "", $matches[1])));
	}
	
	// カード名称・カード番号を取得する
	preg_match_all("/<dd>(.*?)<\/dd>/s", $body, $matches);
	$account['acctname'] = $matches[1][0];
	$account['acctid'] = strip_tags(str_replace("&lowast;", "*", $matches[1][1]));
	
	/*
	// 過去の確定月を取得する
	preg_match_all("/'JAST00016_07M', '(20[0-9]{4})'/", $body, $months, PREG_SET_ORDER);
	while(preg_match("/(JAST[0-9]{5}_[0-9]{2}[A-Z]).*?次へ/", $body, $matches)) {
		$queries = jaccscard_getqueries($body);
		$queries['_TRANID'] .= $matches[1];
		$query = implode("&", $queries);
		$cookie = jaccscard_updatecookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
		$bodies[] = $body;
	}
	*/
	
	// 未確定月を取得する
	if(ENV_BOOL_PENDING_DETAIL) {
		// 実行時間（タイムアウト）を再設定する
		@set_time_limit(ENV_NUM_TIMEOUT);
		
		$queries = jaccscard_getqueries($body);
		$queries['_TRANID'] .= 'JAST00016_19M';
		$query = implode("&", $queries);
		$cookie = jaccscard_updatecookie($head, $cookie);
		list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
		$bodies[] = $body;
		
		while(preg_match("/(JAST[0-9]{5}_[0-9]{2}[A-Z]).*?次へ/", $body, $matches)) {
			$queries = jaccscard_getqueries($body);
			$queries['_TRANID'] .= $matches[1];
			$query = implode("&", $queries);
			$cookie = jaccscard_updatecookie($head, $cookie);
			list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
			$bodies[] = $body;
		}
	}
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// ログアウト
	$queries = jaccscard_getqueries($body);
	$queries['_TRANID'] .= 'JALG00012_00M';
	$query = implode("&", $queries);
	$cookie = jaccscard_updatecookie($head, $cookie);
	list($head, $body) = jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
	
	// DOMツリーを生成
	$ofxdom = new ofxDOM($account['acctid']);
	$f = false;
	foreach($bodies as $body) {
		// 明細をパース
		if($f == false) {
			$f = true;
			$cds = jaccscard_parsedom($body, $paydate, $settings);
		} else {
			$cds = jaccscard_parsedom($body);
		}
		// 明細をセット
		$ofxdom->setTrans($cds);
	}
	$cds_balamt = 0;
	$cds_s = "";
	$cds_e = "";
	$items = $ofxdom->getTrans();
	foreach($items as $item) {
		$dtposted = $item->getElementsByTagName('DTPOSTED')->item(0)->nodeValue;
		// DTSTART, DTENDを取得
		if($cds_s == "") $cds_s = $dtposted;
		$cds_e = $dtposted;
		// 残高を計算
		$cds_balamt += (double)$item->getElementsByTagName('TRNAMT')->item(0)->nodeValue;
	}
	
	// BANKTRANLIST
	$ofxdom->setBankTranList($cds_s, $cds_e);
	
	// 残高を処理
	$cds_balamt = (-1)*$cds_balamt;
	$ofxdom->setLedgerBalance($cds_balamt, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);
	
	//MKGTINFOを設定
	$ofxdom->setMktginfo(implode(ENV_CHR_CONCATENATOR, array($settings["name"], $account["acctname"])));
	
	// XML DOMツリーを文字列に変換
	$xml = $ofxdom->getXML();
	
	$resp["status"] = ENV_NUM_STATUS_SUCCESS;
	$resp["ofx"] = generate_ofx($resp["status"], $xml);
}
return $resp;

// HTTP/1.1
function jaccscard_http11($method, $protocol, $host, $page = "/", $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/\.jaccs\.co\.jp$/", $host) > 0) $ret = http11($method, $protocol, $host, 0, $page, $query, "", $cookie);
	if(preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $ret, $matches) > 0) {
		$urls = parse_url($matches[1]);	
		if($urls["scheme"] != "") $protocol = $urls["scheme"];
		if($urls["host"] != "") $host = $urls["host"];
		if($urls["path"] != "") $page = $urls["path"];
		$query = ($urls["query"] != ""? $urls["query"]: "");
		return jaccscard_http11($method, $protocol, $host, $page, $query, $cookie);
	}
	return explode("\r\n\r\n", $ret, 2);
}

function jaccscard_updatecookie($head, $cookie) {
	$ret = "";
	$cookies = array();
	
	$ckvs = explode(";", $cookie);
	foreach($ckvs as $ckv) {
		list($ck, $cv) = explode("=", $ckv, 2);
		$ck = trim($ck);
		$cv = trim($cv);
		if($ck != "" && $cv != "") $cookies[$ck] = $ck . "=" . $cv;
	}
	
	$cks = array("JSESSIONID", "[0-9]{17}");
	foreach($cks as $ck) if(preg_match_all("/[Ss][Ee][Tt]-[Cc][Oo][Oo][Kk][Ii][Ee][\s\t]*:[\s\t]*(" . $ck . ")=([^;\r\n]*)/", $head, $matches) > 0) $cookies[$matches[1][count($matches[1]) - 1]] = $matches[1][count($matches[1]) - 1] . "=" . $matches[2][count($matches[2]) - 1];
	$ret = implode("; ", $cookies);
	return $ret;
}

function jaccscard_getqueries($body) {
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "name", "MainForm");
	$inputs = parse_tag($forms[$c]["innerHTML"], "input");
	$queries = array();
	foreach($inputs as $input) {
		if($input["name"] != "") {
			$queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		}
	}
	return $queries;
}

function jaccscard_parsedom($str, $paydate = "", $settings = array()) {
	// 明細表読み込み用DOMツリー作成
	$doc = new DOMDocument();
	// <meta … charset=Shift_JIS">の前の<title>が，文字化けの原因となるため，削除
	preg_match("/(<body.*<\/body>)/s", $str, $matches);
	$str = $matches[1];
	// ヘッダ付加
	$str = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><\/head>" . $str . "<\/html>";
	// 文字エンコード変換
	// $str = mb_convert_encoding($str, 'UTF-8', 'sjis-win');
	// &nbsp;
	$str = str_replace("&nbsp;", "", $str);
	// HTMLからDOMツリー作成
	@$doc->loadHTML($str);
	$xpath = new DOMXPath($doc);
	/*
	// 明細表のテーブルを指定
	$rows = $xpath->query("//table[@id='olcreco_target']/tr[@data-pay-id]"); 
	// 確定月か否か flag=false: 確定月, flag=true: 未確定月
	$flag = preg_match('/ggtd web_meisai/', $str);
	*/
	// 確定月か否か flag=false: 確定月, flag=true: 未確定月
	$flag = preg_match('/ggtd web_meisai/', $str);
	if($flag == true) {
		$rows = $xpath->query("//table[@class='ggtd web_meisai']/tr[@data-pay-id]"); 
	} else {
		$rows = $xpath->query("//table[@data-cookie-delete='true']/tr[@data-pay-id]"); 
	}
	
	$ret = "";
	$cds = array();
	$nrow = $rows->length;
	
	$cds_balamt = 0;
	
	for($i=0; $i<$nrow; $i++) {
		$cd = array();
		$row = $rows->item($i);
		$cols = $row->getElementsByTagName('td');
		// 利用明細でない行はスキップ
		if(empty($cols->item(0)->nodeValue)) continue;
		
		$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CREDIT;
		
		// 日付を取得する
		preg_match("/([0-9]{2})\/([0-9]{1,2})\/([0-9]{1,2})/", trim($cols->item(1)->nodeValue), $matches);
		$cd["DTPOSTED"] = sprintf("20%d%02d%02d", $matches[1], $matches[2], $matches[3]);
		
		// 引き落とし月を取得
		/*
		if($flag) {
			$month = substr($cols->item(5)->nodeValue,5);
		} else {
			$month = substr($cols->item(5)->nodeValue,3);
		}
		*/
		$month = substr($cols->item(5)->nodeValue,3);
		
		// トランザクション番号（請求月とデータ種別）を生成する
		$cd["FITID"] = sprintf("%02d0", $month);
		// 摘要を取得する
		$cd["NAME"] = $cols->item(2)->nodeValue;
		$cd["NAME"] = mb_convert_kana($cd["NAME"], "KV", "UTF-8");
		// $cd["NAME"] = preg_replace('/-/', mb_convert_encoding('ー', 'UTF-8', 'sjis-win'), $cd["NAME"]);
		$cd["NAME"] = str_replace('-', 'ー', $cd["NAME"]);
		// 金額を取得する
		if($flag) {
			$cd["TRNAMT"] = (-1)*(double)parse_amount(trim($cols->item(7)->nodeValue));
		} else {
			$cd["TRNAMT"] = (-1)*(double)parse_amount(trim($cols->item(8)->nodeValue));
		}
		array_push($cds, $cd);
		$cds_balamt += $cd["TRNAMT"];
	}
	
	// クレジットカード支払請求を明細に追加する
	if($paydate != "") {
		$cd = array();
		$cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEBIT;
		$cd["DTPOSTED"] = $paydate;
		$cd["NAME"] = $settings["name"];
		$cd["TRNAMT"] = (-1)*$cds_balamt;
		$cd["FITID"] = sprintf("%02d0", $month);
		array_push($cds, $cd);
	}
	
	return $cds;
	
}
?>
