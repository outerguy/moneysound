<?php
/*
benefit401k.inc: SBIベネフィット・システムズのHTMLよりOFXファイルを生成する
Copyright (C) 2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Licensed under the GNU AGPLv3.
*/

// ログイン情報を取得する
$user = $ofxforms["txtUserID"];
$pass = $ofxforms["txtPassword"];

$resp = array();
$ofx = "";

// 実行時間（タイムアウト）を再設定する
@set_time_limit(ENV_NUM_TIMEOUT);

// ホーム画面を取得する
$method = "GET";
$uris = parse_uri($settings["home"]);
$query = "";
$cookie = "";
list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);

// リダイレクトする
$retry = 0;
while($retry++ < ENV_NUM_HTTP11_RETRY) {
	$locations = parse_header($head, "location");
	if(count($locations) > 0) {
		$method = "GET";
		$uris = parse_uri($locations[0], $uris);
		$query = "";
		$cookie = benefit401k_update_cookie($head, $cookie);
		list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);
	}
}

// ログインする
list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "btnLogin", "", $user, $pass); // ログイン

// パスワード変更のお願い画面が表示される場合、次の画面を取得する
if(strpos($body, "パスワード変更のお願い") !== false) {
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "btnHome"); // ホームへ
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
} else if(strpos($body, "前回ログイン日時") === false) {
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
	
	$acctid = $user;
	$acctname = "";
	$bill = "0";
	$balance = "0";
	
	$spans = parse_tag($body, "span");
	$c = parse_tag_search($spans, "id", "D_Header1_lblContributionUnit");
	if($c != -1) $bill = parse_amount(trim(strip_tags($spans[$c]["innerHTML"])));
	
	// 資産状況画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "D_Header1\$uscD_CategoryMenu1\$rptFunctionMenuTable\$_ctl2\$btnMenuData"); // 資産状況
	
	$dtpriceasof = ENV_STR_DATE_TODAY;
	if(preg_match("/([0-9]{4}\/[0-9]{2}\/[0-9]{2})" . preg_quote("　現在") . "/", $body, $matches) != 0) $dtpriceasof = parse_date($matches[1]);
	
	$securities = array();
	$bufs = array();
	
	$tables = parse_tag($body, "table", true);
	$c = parse_tag_search($tables, "id", "grdSyouhinzangaku");
	if($c != -1) {
		$cnt = 0;
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		array_shift($trs);
		foreach($trs as $tr) {
			$tds = parse_tag($tr["innerHTML"], "td");
			$bufs["TYPE"] = trim(strip_tags($tds[0]["innerHTML"])); // 商品タイプ
			$bufs["UNIQUEID"] = (preg_match("/\?SCD=([0-9]+)&/", $tds[1]["innerHTML"], $matches) == 1? $matches[1]: (string)$cnt++); // 商品コード
			$bufs["SECNAME"] = trim(strip_tags($tds[1]["innerHTML"])); // 商品名
			$bufs["UNITS"] = parse_amount(trim(strip_tags($tds[3]["innerHTML"]))); // 口数
			$bufs["UNITPRICE"] = (string)((double)parse_amount(trim(strip_tags($tds[2]["innerHTML"]))) / 10000); // 基準価額
			$bufs["MKTVAL"] = parse_amount(trim(strip_tags($tds[4]["innerHTML"]))); // 時価評価額
			list($amount, $ratio) = explode("<BR>", $tds[6]["innerHTML"], 2);
			$bufs["MEMO"] = parse_amount(trim(strip_tags($amount))); // 損益
			$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
			$bufs["HELDINACCT"] = ENV_STR_OFX_TRNTYPE_CASH; // 現金
			$bufs["POSTYPE"] = "LONG";
			$bufs["DTPRICEASOF"] = $dtpriceasof;
			$bufs["UNIQUEIDTYPE"] = $settings["code"];
			if($bufs["TYPE"] == "待機資金") {
				$balance = $bufs["MKTVAL"];
			} else {
				array_push($securities, $bufs);
			}
			$bufs = array();
		}
	}
	
	// 月ごとの拠出金の詳細を見る画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "grdkyosyutukin\$_ctl2\$Linkbutton2"); // 月ごとの拠出金の詳細を見る
	
	$bills = array();
	$bufs = array();
	
	$tables = parse_tag($body, "table");
	$c = parse_tag_search($tables, "id", "grdKyoshutsu");
	if($c != -1) {
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		foreach($trs as $tr) {
			if($tr["class"] == "tableHeader") continue;
			$tds = parse_tag($tr["innerHTML"], "td");
			if(count($tds) >= 4) {
				$bufs["DTPOSTED"] = parse_date(trim(strip_tags($tds[0]["innerHTML"]))); // 拠出日
				$bufs["NAME"] = trim(strip_tags($tds[1]["innerHTML"])); // 区分
				$bufs["TRNAMT"] = parse_amount(trim(strip_tags($tds[2]["innerHTML"]))); // 拠出金
				$bufs["MEMO"] = trim(strip_tags($tds[3]["innerHTML"])); // 還付有無
				if($bufs["MEMO"] == "") $bufs["MEMO"] = ENV_STR_OFX_MEMO;
				$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_DIRECTDEP;
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				if($bufs["DTPOSTED"] >= ENV_STR_DATE_PASTDAY) array_unshift($bills, $bufs);
				$bufs = array();
			}
		}
	}
	
	// 取引履歴等画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "D_Header1\$btnConfirmDealHistory"); // 取引履歴等
	
	// 取引履歴画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "Linkbutton1"); // 取引履歴
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "Form1");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["PT_SITEI_B"] = "PT_SITEI_B=rdoDates"; // 日付指定
		$queries["oDateSelect1:ddlOne"] = "oDateSelect1:ddlOne=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 0, 4);
		$queries["oDateSelect1:ddlTwo"] = "oDateSelect1:ddlTwo=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["oDateSelect1:ddlThree"] = "oDateSelect1:ddlThree=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 6, 2);
		$queries["oDateSelect2_ddlOne"] = "oDateSelect2:ddlOne=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 0, 4);
		$queries["oDateSelect2:ddlTwo"] = "oDateSelect2:ddlTwo=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["oDateSelect2:ddlThree"] = "oDateSelect2:ddlThree=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 6, 2);
		$queries["__EVENTTARGET"] = "__EVENTTARGET=btnRun"; // 実行
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = benefit401k_update_cookie($head, $cookie);
		list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);
	}
	
	$exchanges = array();
	$bufs = array();
	
	$tables = parse_tag($body, "table");
	$c = parse_tag_search($tables, "id", "grdBusinessHistory");
	if($c != -1) {
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		foreach($trs as $tr) {
			if($tr["class"] == "tableHeader") continue;
			$tds = parse_tag($tr["innerHTML"], "td");
			if(count($tds) >= 7) {
				$bufs["DTTRADE"] = parse_date(trim(strip_tags($tds[0]["innerHTML"]))); // 約定日
				$bufs["SECNAME"] = trim(strip_tags($tds[2]["innerHTML"])); // 運用商品名
				$bufs["UNITS"] = parse_amount(trim(strip_tags($tds[3]["innerHTML"]))); // 数量
				$bufs["UNITPRICE"] = parse_amount(trim(strip_tags($tds[4]["innerHTML"]))); // 約定単価
				if($bufs["UNITPRICE"] == "-") $bufs["UNITPRICE"] = "1";
				$bufs["TOTAL"] = parse_amount(trim(strip_tags($tds[5]["innerHTML"]))); // 受渡金額
				list($buysell, $memo) = explode(")", trim(strip_tags($tds[6]["innerHTML"])), 2); // 取引区分
				switch($buysell) {
				case "売":
					$bufs["BUYTYPE"] = ENV_STR_OFX_SELL;
					$bufs["UNITS"] = "-" . $bufs["UNITS"];
					break;
				case "買":
				default:
					$bufs["BUYTYPE"] = ENV_STR_OFX_BUY;
					$bufs["TOTAL"] = "-" . $bufs["TOTAL"];
					break;
				}
				$bufs["MEMO"] = $memo;
				$bufs["CATEGORY"] = ENV_STR_OFX_FUND; // 投資信託
				$bufs["UNIQUEIDTYPE"] = $settings["code"];
				$bufs["SUBACCTSEC"] = ENV_STR_OFX_CASH; // 現金
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				// 約定単価を補正する
				$bufs["UNITPRICECALC"] = ((integer)$bufs["UNITS"] === 0? 0: sprintf("%.6f", abs((integer)$bufs["TOTAL"]) / abs((integer)$bufs["UNITS"])));
				if($bufs["UNITPRICE"] != substr($bufs["UNITPRICECALC"], 0, strlen($bufs["UNITPRICE"]))) $bufs["UNITPRICE"] = $bufs["UNITPRICECALC"];
				array_unshift($exchanges, $bufs);
				$bufs = array();
			}
		}
	}
	
	// 取引履歴等画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "D_Header1\$btnConfirmDealHistory"); // 取引履歴等
	
	// 事務手数料履歴画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "btnBusinessFee"); // 事務手数料履歴
	
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "Form1");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		$queries["PT_SITEI_B"] = "PT_SITEI_B=rdoDateSpecification"; // 日付指定
		$queries["uscDateSelect1:ddlOne"] = "uscDateSelect1:ddlOne=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 0, 4);
		$queries["uscDateSelect1:ddlTwo"] = "uscDateSelect1:ddlTwo=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 4, 2);
		$queries["uscDateSelect1:ddlThree"] = "uscDateSelect1:ddlThree=" . (string)(integer)substr(ENV_STR_DATE_PASTDAY, 6, 2);
		$queries["uscDateSelect2:ddlOne"] = "uscDateSelect2:ddlOne=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 0, 4);
		$queries["uscDateSelect2:ddlTwo"] = "uscDateSelect2:ddlTwo=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 4, 2);
		$queries["uscDateSelect2:ddlThree"] = "uscDateSelect2:ddlThree=" . (string)(integer)substr(ENV_STR_DATE_TODAY, 6, 2);
		$queries["__EVENTTARGET"] = "__EVENTTARGET=btnRun"; // 実行
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = benefit401k_update_cookie($head, $cookie);
		list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);
	}
	
	$fees = array();
	$bufs = array();
	
	$tables = parse_tag($body, "table");
	$c = parse_tag_search($tables, "id", "grdBusinessFeeHistory");
	if($c != -1) {
		$trs = parse_tag($tables[$c]["innerHTML"], "tr");
		foreach($trs as $tr) {
			if($tr["class"] == "tableHeader") continue;
			$tds = parse_tag($tr["innerHTML"], "td");
			if(count($tds) >= 6) {
				$bufs["DTPOSTED"] = parse_date(trim(strip_tags($tds[0]["innerHTML"]))); // 約定日
				$bufs["KOKKIREN"] = parse_amount(trim(strip_tags($tds[1]["innerHTML"]))); // 国基連手数料
				$bufs["UNKAN"] = parse_amount(trim(strip_tags($tds[2]["innerHTML"]))); // 運営管理手数料
				$bufs["JIMUITAKU"] = parse_amount(trim(strip_tags($tds[3]["innerHTML"]))); // 事務委託手数料
				$bufs["TRNAMT"] = "-" . parse_amount(trim(strip_tags($tds[4]["innerHTML"]))); // 手数料合計
				$bufs["NAME"] = trim(strip_tags($tds[5]["innerHTML"])); // 事務手数料区分
				$bufs["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_FEE;
				$bufs["MEMO"] = ENV_STR_OFX_MEMO;
				$bufs["SUBACCTFUND"] = ENV_STR_OFX_CASH; // 現金
				array_unshift($fees, $bufs);
				$bufs = array();
			}
		}
	}
	
	$banks = array();
	$buf = array();
	
	foreach($bills as $bill) array_push($banks, $bill);
	foreach($fees as $fee) array_push($banks, $fee);
	
	// asort()が安定でないため、安定にソートする
	// asort($banks);
	$counts = array();
	foreach($banks as $bank) {
		if(isset($counts[$bank["DTPOSTED"]]) === false) $counts[$bank["DTPOSTED"]] = array();
		array_push($counts[$bank["DTPOSTED"]], $bank);
	}
	$cnts = array_keys($counts);
	sort($cnts);
	$banks = array();
	foreach($cnts as $cnt) foreach($counts[$cnt] as $count) array_push($banks, $count);
	
	// 運用商品一覧画面を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "D_Header1\$uscD_CategoryMenu1\$rptFunctionMenuTable\$_ctl3\$btnMenuData"); // 運用商品一覧
	
	$investments = array();
	$bufs = array();
	
	$tds = parse_tag($body, "td");
	foreach($tds as $td) {
		if($td["width"] == "280px") {
			$as = parse_tag($td["innerHTML"], "a");
			if(count($as) == 1 && preg_match("/[0-9]{10}/", $as[0]["onclick"], $matches) != 0) {
				$bufs["UNIQUEID"] = $matches[0];
				$bufs["SECNAME"] = $as[0]["innerHTML"];
				array_push($investments, $bufs);
				$bufs = array();
			}
		}
	}
	
	$body_old = $body;
	
	// 運用商品ラインナップ（CSV）を取得する
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "btnProductLineupCSV"); // 運用商品ラインナップ（CSV）
	
	// 運営管理機関名を取得する
	$csvs = parse_csv(mb_convert_string($body));
	foreach($csvs as $csv) {
		if($csv[1] == "元本変動" && $acctname == "") {
			$acctname = ENV_CHR_CONCATENATOR . trim(strip_tags($csv[6])); // 商品提供会社
			break;
		}
	}
	
	$body = $body_old;
	
	// ログアウトする
	list($head, $body, $uris, $cookie) = benefit401k_post($head, $body, $uris, $cookie, "D_Header1\$btnLogout"); // ログアウト
	
	// 実行時間（タイムアウト）を再設定する
	@set_time_limit(ENV_NUM_TIMEOUT);
	
	// INVTRANLIST（投資）
	$invtranlist = "";
	$invtranlist .= "<DTSTART>" . ENV_STR_DATE_PASTDAY . ENV_STR_OFX_TZ . "</DTSTART>";
	$invtranlist .= "<DTEND>" . ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ . "</DTEND>";
	$invtranlist .= "\r\n";
	
	$cd_date = "";
	$cd_num = 0;
	
	foreach($exchanges as $exchange) {
		if($exchange["DTTRADE"] == $cd_date) $cd_num++; else $cd_num = 0;
		$fitid = $exchange["DTTRADE"] . "0000001" . sprintf("%05d", $cd_num);
		
		$uniqueid = "";
		foreach($investments as $investment) {
			if(mb_convert_kana(str_replace("・", "", $investment["SECNAME"]), "ask", "UTF-8") == $exchange["SECNAME"]) {
				$uniqueid = $investment["UNIQUEID"];
				break;
			}
		}
		if($uniqueid == "") $uniqueid = mb_convert_uniqueid($exchange["SECNAME"]);
		
		// 売買の明細を生成する
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
		// SECLIST
		$seclist .= "<" . $security["CATEGORY"] . "INFO><SECINFO>";
		$seclist .= "<SECID><UNIQUEID>" . $security["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
		$seclist .= "<SECNAME>" . $security["SECNAME"] . "</SECNAME>";
		$seclist .= "</SECINFO></" . $security["CATEGORY"] . "INFO>";
		$seclist .= "\r\n";
		
		$invposlist .= "<POS" . $security["CATEGORY"] . "><INVPOS>";
		$invposlist .= "<SECID><UNIQUEID>" . $security["UNIQUEID"] . "</UNIQUEID><UNIQUEIDTYPE>" . $security["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
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
	
	foreach($exchanges as $exchange) {
		$secname = "";
		$uniqueid = "";
		foreach($investments as $investment) {
			if(mb_convert_kana($investment["SECNAME"], "ask", "UTF-8") == $exchange["SECNAME"]) {
				$secname = $investment["SECNAME"];
				$uniqueid = $investment["UNIQUEID"];
				break;
			}
		}
		if($secname == "") $secname = $exchange["SECNAME"];
		if($uniqueid == "") $uniqueid = mb_convert_uniqueid($exchange["SECNAME"]);
		
		if(strpos($seclist, $secname) === false) {
			// SECLIST
			$seclist .= "<!-- SOLD -->";
			$seclist .= "<" . $exchange["CATEGORY"] . "INFO><SECINFO>";
			$seclist .= "<SECID><UNIQUEID>" . $uniqueid . "</UNIQUEID><UNIQUEIDTYPE>" . $exchange["UNIQUEIDTYPE"] . "</UNIQUEIDTYPE></SECID>";
			$seclist .= "<SECNAME>" . $secname . "</SECNAME>";
			$seclist .= "</SECINFO></" . $exchange["CATEGORY"] . "INFO>";
			$seclist .= "\r\n";
		}
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
	$invstmtmsgsrsv1 .= "<AVAILCASH>" . $balance . "</AVAILCASH>";
	$invstmtmsgsrsv1 .= "<MARGINBALANCE>" . $balance . "</MARGINBALANCE>";
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

// HTTP/1.1
function benefit401k_http11($method, $uris, $query = "", $cookie = "") {
	$ret = "INVALID HOST";
	if(preg_match("/^www.benefit401k\.com$/", $uris["host"]) > 0) $ret = http11(strtoupper($method), $uris["scheme"], $uris["host"], 0, $uris["path"], $query, "", $cookie);
	return explode("\r\n\r\n", $ret, 2);
}

function benefit401k_update_cookie($head, $cookie) {
	return update_cookie(array("ASP.NET_SessionId", "SVRID", "sto-id", "Auth"), parse_header($head, "set-cookie"), $cookie);
}

function benefit401k_post($head, $body, $uris, $cookie, $eventtarget, $imagebutton = "", $user = "", $pass = "") {
	$forms = parse_tag($body, "form");
	$c = parse_tag_search($forms, "id", "Form1");
	if($c == -1) $c = parse_tag_search($forms, "id", "JP_Info_FileApply_Select");
	if($c != -1) {
		$inputs = parse_tag($forms[$c]["innerHTML"], "input");
		$queries = array();
		foreach($inputs as $input) if($input["name"] != "" && $input["type"] != "image") $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
		if($user !== "") $queries["txtUserID"] = "txtUserID=" . $user;
		if($pass !== "") $queries["txtPassword"] = "txtPassword=" . $pass;
		if($imagebutton !== "") {
			$queries[urlencode($imagebutton) . ".x"] = urlencode($imagebutton) . ".x=0";
			$queries[urlencode($imagebutton) . ".y"] = urlencode($imagebutton) . ".y=0";
		}
		$queries["__EVENTTARGET"] = "__EVENTTARGET=" . $eventtarget;
		if(isset($queries["HdnBtn"]) == true) unset($queries["HdnBtn"]);
		if(isset($queries["HdnSiteBtn"]) == true) unset($queries["HdnSiteBtn"]);
		
		$method = $forms[$c]["method"];
		$uris = parse_uri($forms[$c]["action"], $uris);
		$query = implode("&", $queries);
		$cookie = benefit401k_update_cookie($head, $cookie);
		list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);
	}
	
	// リダイレクトする
	$retry = 0;
	while($retry++ < ENV_NUM_HTTP11_RETRY) {
		$locations = parse_header($head, "location");
		if(count($locations) > 0) {
			$method = "GET";
			$uris = parse_uri($locations[0], $uris);
			$query = "";
			$cookie = benefit401k_update_cookie($head, $cookie);
			list($head, $body) = benefit401k_http11($method, $uris, $query, $cookie);
		}
	}
	
	return array($head, $body, $uris, $cookie);
}

?>
