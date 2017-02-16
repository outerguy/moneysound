<?php
/*
server.inc: サーバー側の定数・共通関数を定義する
Copyright (C) 2008-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU AGPLv3 and Beatrek Origin License.

class ofxDOM
Copyright (C) 2012-2015 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Licensed under the GNU AGPLv3.
*/

define("ENV_BOOL_PENDING_DETAIL", false); // （クレジットカードの）未確定明細を出力するか否か
define("ENV_BOOL_ADD_RISKBASE", true); // リスクベース認証成功時、金融機関に接続情報を登録するか否か

define("ENV_NUM_HTTP11_SIZE_BUF", 1024);
define("ENV_NUM_HTTP11_RETRY", 8);

define("ENV_NUM_TIMEOUT", 30);

define("ENV_FILE_DIR_SERVER", "./server/");
define("ENV_FILE_DIR_LOG", "./log/");

define("ENV_FILE_EXT_INC", ".inc");
define("ENV_FILE_EXT_OFX", ".ofx");
define("ENV_FILE_EXT_LOG", ".log");

define("ENV_STR_SESSION_PADDING", " ");

define("ENV_STR_ACCTTYPE_SAVINGS", "SAVINGS"); // 普通預金口座
define("ENV_STR_ACCTTYPE_CHECKING", "CHECKING"); // 当座預金口座

define("ENV_STR_DATE_TODAY", date("Ymd")); // 本日
define("ENV_STR_DATE_PASTDAY", date("Ymd", strtotime(date("Y-m-01") . " -1 month"))); // 前月1日

define("ENV_STR_OFX_CURRENCY_JPY", "JPY"); // 通貨
define("ENV_STR_OFX_TZ", "000000[+9:JST]"); // 時分秒・タイムゾーン
define("ENV_STR_OFX_NODATA", "<!-- NO DATA -->"); // 空

define("ENV_STR_OFX_STOCK", "STOCK"); // 株式
define("ENV_STR_OFX_FUND", "MF"); // 投資信託
define("ENV_STR_OFX_DEBT", "DEBT"); // 債券
define("ENV_STR_OFX_CASH", "CASH"); // 預り金
define("ENV_STR_OFX_BUY", "BUY"); // 買
define("ENV_STR_OFX_SELL", "SELL"); // 売
define("ENV_STR_OFX_REINVEST", "REINVEST"); // 再投資
define("ENV_STR_OFX_MEMO", "-"); // メモのデフォルト値

define("ENV_STR_OFX_TRNTYPE_CREDIT", "CREDIT"); // Generic credit
define("ENV_STR_OFX_TRNTYPE_DEBIT", "DEBIT"); // Generic debit
define("ENV_STR_OFX_TRNTYPE_INT", "INT"); // Interest earned or paid
define("ENV_STR_OFX_TRNTYPE_DIV", "DIV"); // Dividend
define("ENV_STR_OFX_TRNTYPE_FEE", "FEE"); // FI fee
define("ENV_STR_OFX_TRNTYPE_SRVCHG", "SRVCHG"); // Service charge
define("ENV_STR_OFX_TRNTYPE_DEP", "DEP"); // Deposit
define("ENV_STR_OFX_TRNTYPE_ATM", "ATM"); // ATM debit or credit
define("ENV_STR_OFX_TRNTYPE_POS", "POS"); // Point of sale debit or credit
define("ENV_STR_OFX_TRNTYPE_XFER", "XFER"); // Transfer
define("ENV_STR_OFX_TRNTYPE_CHECK", "CHECK"); // Check
define("ENV_STR_OFX_TRNTYPE_PAYMENT", "PAYMENT"); // Electronic payment
define("ENV_STR_OFX_TRNTYPE_CASH", "CASH"); // Cash withdrawal
define("ENV_STR_OFX_TRNTYPE_DIRECTDEP", "DIRECTDEP"); // Direct deposit
define("ENV_STR_OFX_TRNTYPE_DIRECTDEBIT", "DIRECTDEBIT"); // Merchant initiated debit
define("ENV_STR_OFX_TRNTYPE_REPEATPMT", "REPEATPMT"); // Repeating payment/standing order
define("ENV_STR_OFX_TRNTYPE_OTHER", "OTHER"); // Other

define("ENV_STR_OFX_SEVERITY_INFO", "INFO"); // Informational only
define("ENV_STR_OFX_SEVERITY_WARN", "WARN"); // Some problem with the request occurred but a valid response still present
define("ENV_STR_OFX_SEVERITY_ERROR", "ERROR"); // A problem severe enough that response could not be made

define("ENV_STR_OFX_CODE_STOCK", "JP:SIC"); // 証券情報センター
define("ENV_STR_OFX_CODE_FUND", "JP:ITAJ"); // The Investment Trusts Association, Japan
define("ENV_STR_OFX_CODE_HC", "JP:HC"); // PURSE Hashed Code

define("ENV_STR_OFX_CODE_SUCCESS", "0"); // 成功
define("ENV_STR_OFX_CODE_ADDITION", "3000"); // 追加認証
define("ENV_STR_OFX_CODE_FAILURE", "15500"); // 認証エラー
define("ENV_STR_OFX_CODE_MAINTENANCE", "16503"); // メンテナンスエラー
define("ENV_STR_OFX_CODE_CAUTION", "15511"); // 情報取得エラー

define("ENV_CHR_SEPARATOR", "\t");
define("ENV_CHR_CONCATENATOR", "　");

function env_dlog($str) {
	if(ENV_BOOL_DEBUG == true) {
		list($usec, $sec) = explode(" ", (string)microtime());
		@file_put_contents(ENV_FILE_DIR_LOG . date("YmdHis", (integer)$sec) . $usec . ENV_FILE_EXT_LOG, $str);
	}
	return ENV_BOOL_DEBUG;
}

function sess_encode($str) {
	return urlencode(base64_encode(gzdeflate($str, 9)));
}

function sess_decode($str) {
	return gzinflate(base64_decode(urldecode($str)));
}

function mb_convert_uniqueid($str) {
	return strtoupper(md5($str));
}

function mb_convert_uniquename($str) {
	return strtoupper(mb_convert_kana(str_replace(array("　", "－", "・"), array("", "", ""), $str), "ask", "UTF-8"));
}

function mb_convert_string($str) {
	return str_replace("−", "－", mb_convert_encoding($str, "UTF-8", "Shift_JIS"));
}

class ofxDOM {
	private $dom;
	private $type;
	private $acctfrom;
	private $tranlist;
	private $poslist;
	private $seclist;
	private $stmtrs;
	private $balance;
	
	// XML DOMツリーを作成する
	// $ofxdom = new ofxDOM("CREDITCARD");
	public function __construct($type, $mktginfo="") {
		$this->type = $type;
		$this->dom = simplexml_load_string('<?xml version="1.0" encoding="UTF-8"?' . '><OFX/>');
		switch($type) {
			case "CREDITCARD":
				$msg = $this->dom->addChild('CREDITCARDMSGSRSV1');
				$trnrs = $msg->addChild('CCSTMTTRNRS');
				$trnrs->addChild('TRNUID', '0');
				$status = $trnrs->addChild('STATUS');
				$status->addChild('CODE', '0');
				$status->addChild('SEVERITY', 'INFO');

				$stmtrs = $trnrs->addChild('CCSTMTRS');
				$this->stmtrs = $stmtrs;
				$stmtrs->addChild('CURDEF', ENV_STR_OFX_CURRENCY_JPY);
				$this->acctfrom = $stmtrs->addChild('CCACCTFROM');
				$this->tranlist = $stmtrs->addChild('BANKTRANLIST');
				$this->balance = $stmtrs->addChild('LEDGERBAL');
				break;
			case "INVSTMT":
				$msg = $this->dom->addChild('INVSTMTMSGSRSV1');
				$trnrs = $msg->addChild('INVSTMTTRNRS');
				$trnrs->addChild('TRNUID', '0');
				$status = $trnrs->addChild('STATUS');
				$status->addChild('CODE', '0');
				$status->addChild('SEVERITY', 'INFO');

				$stmtrs = $trnrs->addChild('INVSTMTRS');
				$this->stmtrs = $stmtrs;
				$stmtrs->addChild('DTASOF', ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);
				$stmtrs->addChild('CURDEF', ENV_STR_OFX_CURRENCY_JPY);
				$this->acctfrom = $stmtrs->addChild('INVACCTFROM');
				$this->tranlist = $stmtrs->addChild('INVTRANLIST');
				$this->poslist = $stmtrs->addChild('INVPOSLIST');
				$this->balance = $stmtrs->addChild('INVBAL');

				$msg = $this->dom->addChild('SECLISTMSGSRSV1');
				$trnrs = $msg->addChild('SECLISTTRNRS');
				$trnrs->addChild('TRNUID', '0');
				$status = $trnrs->addChild('STATUS');
				$status->addChild('CODE', '0');
				$status->addChild('SEVERITY', 'INFO');
				$this->seclist = $msg->addChild('SECLIST');
				break;
			// 銀行、前払式帳票（電子マネー）
			case "BANK":
				$msg = $this->dom->addChild('BANKMSGSRSV1');
				$trnrs = $msg->addChild('STMTTRNRS');
				$trnrs->addChild('TRNUID', '0');
				$status = $trnrs->addChild('STATUS');
				$status->addChild('CODE', '0');
				$status->addChild('SEVERITY', 'INFO');

				$stmtrs = $trnrs->addChild('STMTRS');
				$this->stmtrs = $stmtrs;
				$stmtrs->addChild('CURDEF', ENV_STR_OFX_CURRENCY_JPY);
				$this->acctfrom = $stmtrs->addChild('BANKACCTFROM');
				$this->tranlist = $stmtrs->addChild('BANKTRANLIST');
				$this->balance = $stmtrs->addChild('LEDGERBAL');
				break;
			default:
				throw new Exception('第1引数はCREDITCARD, INVSTMT, BANKのいずれか');
				break;
		}
		$this->tranlist->addChild('DTSTART');
		$this->tranlist->addChild('DTEND');
		$this->stmtrs->addChild('MKTGINFO', $mktginfo);
	}

	// 口座情報
	public function setAcctfrom($array) {
		foreach($array as $key => $value) {
			$this->acctfrom->addChild($key, $value);
		}
	}
	
	// 明細を追記する
	public function addTran($cd) {
		if ($this->type == 'INVSTMT') {
			$parent = $this->tranlist->addChild('INVBANKTRAN');
			$stmttrn = $parent->addChild('STMTTRN');
			$parent->addChild('SUBACCTFUND', 'CASH');
		} else {
			$parent = $this->tranlist;
			$stmttrn = $parent->addChild('STMTTRN');
		}
		$array = array('TRNTYPE', 'DTPOSTED', 'TRNAMT', 'FITID');
		foreach($array as $key) {
			$stmttrn->addChild($key, $cd[$key]);
		}
		// 特殊文字に関するPHPのバグ（https://bugs.php.net/bug.php?id=39521）を回避
		$stmttrn->addChild('NAME');
		$stmttrn->NAME = $cd['NAME'];
		$stmttrn->addChild('MEMO');
		$stmttrn->MEMO = $cd['MEMO'];
	}

	// 取引履歴を書き込む
	public function addTrade($ct) {
		if ($ct['BUYTYPE'] == ENV_STR_OFX_REINVEST) {
			$array = array('INCOMETYPE', 'TOTAL', 'SUBACCTSEC', 'UNITS', 'UNITPRICE');
			$parent = $this->tranlist->addChild('REINVEST');
		} else {
			$array = array('UNITS', 'UNITPRICE');
			switch($ct['CATEGORY']) {
			case ENV_STR_OFX_STOCK:
				array_push($array, 'TAXES', 'FEES');
				break;
			case ENV_STR_OFX_FUND:
				array_push($array, 'COMMISSION', 'TAXES');
				break;
			case ENV_STR_OFX_CASH:
			default:
				break;
			}
			array_push($array, 'TOTAL', 'SUBACCTSEC', 'SUBACCTFUND');
			$tran = $this->tranlist->addChild($ct['BUYTYPE'] . $ct['CATEGORY']);
			$parent = $tran->addChild('INV' . $ct['BUYTYPE']);
			$tran->addChild($ct["BUYTYPE"] . 'TYPE', $ct['BUYTYPE']);
		}
		$invtran = $parent->addChild('INVTRAN');
		$invtran->addChild('FITID', $ct['FITID']);
		$invtran->addChild('DTTRADE', $ct['DTTRADE']);

		$secid = $parent->addChild('SECID');
		$secid->addChild('UNIQUEID', $ct['UNIQUEID']);
		$secid->addChild('UNIQUEIDTYPE', $ct['UNIQUEIDTYPE']);

		foreach($array as $key) {
			$parent->addChild($key, $ct[$key]);
		}
	}

	public function getTrans() {
		return $this->tranlist->STMTTRN;
	}
	
	public function setDateRange($dtstart, $dtend) {
		$this->tranlist->DTSTART = $dtstart;
		$this->tranlist->DTEND = $dtend;
	}

	// 証券残高を書き込む
	public function addPos($cl) {
		$pos = $this->poslist->addChild('POS' . $cl['CATEGORY']);
		$invpos = $pos->addChild('INVPOS');

		$secid = $invpos->addChild('SECID');
		$secid->addChild('UNIQUEID', $cl['UNIQUEID']);
		$secid->addChild('UNIQUEIDTYPE', $cl['UNIQUEIDTYPE']);

		$array = array('HELDINACCT', 'POSTYPE', 'UNITS', 'UNITPRICE',
					   'MKTVAL', 'DTPRICEASOF', 'MEMO');
		foreach($array as $key) {
			$invpos->addChild($key, $cl[$key]);
		}
	}

	// SECLIST
	public function addSec($cl) {
		$info = $this->seclist->addChild($cl['CATEGORY'] . 'INFO');
		$secinfo = $info->addChild('SECINFO');

		$secid = $secinfo->addChild('SECID');
		$secid->addChild('UNIQUEID', $cl['UNIQUEID']);
		$secid->addChild('UNIQUEIDTYPE', $cl['UNIQUEIDTYPE']);

		$secinfo->addChild('SECNAME', $cl['SECNAME']);

		if($cl["CATEGORY"] == ENV_STR_OFX_DEBT) {
			$info->addChild('PARVALUE', $cl['PARVALUE']);
			$info->addChild('DEBTTYPE', $cl['DEBTTYPE']);
			$info->addChild('COUPONRT', $cl['COUPONRT']);
			$info->addChild('DTMAT', $cl['DTMAT']);
		}
	}

	public function setBalance($array) {
		foreach($array as $key => $value) {
			$this->balance->addChild($key, $value);
		}
	}

	// Deprecated.
	public function setFitid() {
		$acctid = $this->acctfrom->ACCTID;
		$acctid = substr($acctid, strlen($acctid) - 4);
		$cd_nums = array();
		$fitids = $this->dom->xpath('//FITID');
		foreach ($fitids as $fitid) {
			$parent = $fitid->xpath('..')[0];
			if ($this->type == 'INVSTMT') {
				$dt = $parent->DTTRADE;
			} else {
				$dt = $parent->DTPOSTED;
			}
			$dt = substr($dt, 0, 8);
			if (array_key_exists($dt, $cd_nums)) {
				$cd_nums[$dt]++;
			} else {
				$cd_nums[$dt] = 0;
			}
			// 口座種目は必要ないため、当面0000とする
			$parent->FITID = sprintf("%s0000%s%05d", $dt, $parent->FITID, $cd_nums[$dt]);
		}
	}

	public function getXML() {
		// Pretty-printのためだけにDOMDocumentを利用する
		$dom = dom_import_simplexml($this->dom)->ownerDocument;
		$dom->formatOutput = TRUE;
		$xml = $dom->saveXML();
		// XMLのヘッダは削除
		$xml = preg_replace('/\<\?xml.*\?\>/', '', $xml);
		// <OFX>と</OFX>を削除する
		$xml = preg_replace('/<[\/]?OFX>/', '', $xml);
		// 改行コードをCRLFに変換する
		$xml = str_replace("\n", "\r\n", $xml);
		return $xml;
	}
}

// 金額をパースする
function parse_amount($str) {
	$buf = str_replace(array(",", "△", "▲", "千", "万"), array("", "-", "-", "000", "0000"), trim($str));
	$ret = preg_replace("/[^0-9\-\.]/", "", $buf);
	if($ret == "") $ret = "0";
	return $ret;
}

// 日付をパースする
function parse_date($str) {
	list($dy, $dm, $dd, $dummy) = explode("/", str_replace(array("年", "月", "日", "-", ".", "T", " "), array("/", "/", "/", "/", "/", "/", "/"), trim($str)), 4);
	$dy = (integer)$dy;
	if($dy < 1988) $dy += 1988;
	$dm = (integer)$dm;
	$dd = (integer)$dd;
	return sprintf("%04d%02d%02d", $dy, $dm, $dd);
}

// フォーム経由で送受信されるパラメータをパースする
function parse_param($str) {
	return str_replace(array("%2A", "%40"), array("*", "@"), urlencode(urldecode(str_replace("+", "%2B", $str))));
}

// 指定された開始タグから終了タグまでのHTML、およびタグの属性を取得する
function parse_tag($html, $tag, $recursive = false) {
	$ret = array();
	$buf = $html;
	
	// HTML以外のタグ内の要素を取り除く
	$strips = array("script", "style");
	foreach($strips as $strip) if($strip != strtolower($tag)) $buf = preg_replace("/<" . preg_quote($strip) . "(?!\w)[^>]*>.*?<\/" . preg_quote($strip) . ">/ims", "", $buf);
	
	$c = preg_match_all("/<(\/?)" . preg_quote($tag) . "((?!\w)[^>]*)>/ims", $buf, $matches);
	if(array_search("/", $matches[1]) !== false) {
		// 終了タグが存在する場合
		$bufs = array(); // タグのネスト構造を保持する配列
		$nest = 0; // タグのネスト数
		$cnt = 0; // 開始タグと終了タグのセット数
		$pos = 0; // 検索開始位置（文字列の先頭からのバイト数）
		// 「<!-- tag></tag -->」のような終了タグ（コメント）を除く
		for($i = 0; $i < $c; $i++) if($matches[2][$i] != "--") {
			if($matches[1][$i] == "") {
				// 開始タグの場合、開始タグを取得する
				$bufs[$nest]["tag"] = $matches[0][$i];
				// 開始タグの開始位置を取得する
				$bufs[$nest]["start"] = strpos($buf, $bufs[$nest]["tag"], $pos);
				// 検索開始位置を開始タグの終了位置に更新する
				$pos = $bufs[$nest]["start"] + strlen($bufs[$nest]["tag"]);
				$bufs[$nest]["pos"] = $pos;
				// ネスト数を増やす
				$nest++;
			} else {
				// 終了タグの場合、ネスト数を減らす
				$nest--;
				// 検索開始位置を更新する
				$pos = strpos($buf, $matches[0][$i], $pos);
				if(($recursive == false && $nest == 0) || $recursive == true) {
					// $recursiveがfalseかつネスト数が0、または$recursiveがtrueの場合
					// 開始タグの属性を取得する
					$ret[$cnt] = parse_tag_attributes($bufs[$nest]["tag"], $tag);
					// 開始タグから終了タグまでの中身を切り出す
					$ret[$cnt]["innerHTML"] = substr($buf, $bufs[$nest]["pos"], ($pos - $bufs[$nest]["pos"]));
					// 検索開始位置を終了タグの終了位置に更新する
					$pos += strlen($matches[0][$i]);
					// 開始タグと終了タグを含めて切り出す
					$ret[$cnt]["match"] = substr($buf, $bufs[$nest]["start"], ($pos - $bufs[$nest]["start"]));
					// 開始タグと終了タグのセット数を増やす
					$cnt++;
				}
			}
		}
	} else {
		// 終了タグが存在しない場合、開始タグの属性を取得する
		for($i = 0; $i < $c; $i++) {
			$ret[$i] = parse_tag_attributes($matches[0][$i], $tag);
			$ret[$i]["innerHTML"] = "";
			$ret[$i]["match"] = $matches[0][$i];
		}
	}
	return $ret;
}

// 指定されたタグの属性を取得する
function parse_tag_attributes($html, $tag) {
	$ret = array();
	
	// タグの属性を切り出す
	$s = strlen($tag) + 1;
	$e = strlen($html) - $s - 1;
	$buf = substr($html, $s, $e);
	
	// タブ・改行コードをスペースに置換する
	$buf = str_replace(array("\t", "\r", "\n"), array(" ", " ", " "), $buf);
	
	// =の前後のスペースを取り除く
	$buf = preg_replace("/\s*=\s*/", "=", $buf);
	
	// 右辺が""か''で括られていない場合、末尾に改行コードを追加する
	$buf = preg_replace("/(=)([^\'\"])([^\\1\s]++)/", "\\1\\2\\3\n", $buf);
	
	// 右辺が""か''で括られている場合、取り除き、末尾に改行コードを追加する
	$buf = preg_replace("/(=)([\'\"])([^\\2]*?)\\2/", "\\1\\3\n", $buf);
	
	// 改行コードで分割する
	$kvs = explode("\n", $buf);
	foreach($kvs as $kv) {
		$kv = ltrim($kv);
		if($kv != "") {
			// key=valueを抽出する（keyは英小文字に変換する）
			$p = strpos($kv, "=");
			if($p !== false) {
				// key=value
				$k = strtolower(substr($kv, 0, $p));
				$v = substr($kv, $p + 1);
				if($v === false) $v = "";
			} else {
				// valueが存在しない場合、key=keyとする
				$k = strtolower($kv);
				$v = $k;
			}
			if($k != "/") $ret[$k] = $v;
		}
	}
	return $ret;
}

// 指定されたタグ配列より属性が一致（$key=$value）するタグの配列番号を取得する
function parse_tag_search($tags, $key, $value) {
	$ret = -1;
	$c = count($tags);
	for($i = 0; $i < $c; $i++) if($tags[$i][$key] == $value) {
		$ret = $i;
		break;
	}
	return $ret;
}

// $headに一致するHTTPヘッダーの配列を取得する
function parse_header($http, $head) {
	$ret = array();
	$c = preg_match_all("/" . preg_quote($head) . "[\s\t]*:[\s\t]*([^\r\n]*)/i", $http, $matches);
	for($i = 0; $i < $c; $i++) array_push($ret, $matches[1][$i]);
	return $ret;
}

// $uri_nextを$urisからの絶対・相対パスURIとしてパースする
function parse_uri($uri_next, $uris = array()) {
	$ret = $uris;
	if(isset($ret["path"]) == false) {
		$ret["path"] = "/";
	} else {
		$c = strpos($ret["path"], "?");
		if($c !== false) $ret["path"] = substr($ret["path"], 0, $c);
		$c = strrpos($ret["path"], "/");
		if($c !== false) $ret["path"] = substr($ret["path"], 0, $c + 1);
	}
	if(isset($ret["query"]) == true) unset($ret["query"]);
	
	$uri_nexts = parse_url($uri_next);
	if(isset($uri_nexts["scheme"]) == true) $ret["scheme"] = $uri_nexts["scheme"];
	if(isset($uri_nexts["host"]) == true) $ret["host"] = $uri_nexts["host"];
	
	$prevs = explode("/", (substr($uri_nexts["path"], 0, 1) == "/"? "": $ret["path"]) . $uri_nexts["path"]);
	$nexts = array();
	foreach($prevs as $prev) if($prev == "..") array_pop($nexts); else if($prev != ".") array_push($nexts, $prev);
	$ret["path"] = implode("/", $nexts);
	
	if(isset($uri_nexts["query"]) == true && $uri_nexts["query"] != "") $ret["path"] .= "?" . html_entity_decode($uri_nexts["query"]);
	
	return $ret;
}

// CSV形式のデータをパースする
function parse_csv($csv, $delimiter = ",", $enclosure = "\"") {
	$ret = array();
	$de = substr($delimiter, 0, 1);
	$en = substr($enclosure, 0, 1);
	
	$lines = array();
	switch(gettype($csv)) {
	case "array":
		foreach($csv as $line) array_push($lines, rtrim($line, "\r\n"));
		break;
	case "string":
	default:
		$buf = preg_replace("/[\r\n]+/", "\n", rtrim($csv, "\r\n"));
		if($buf != "") $lines = explode("\n", $buf);
		break;
	}
	
	foreach($lines as $line) {
		$buf = "";
		do {
			$es = strpos($line, $en, 0);
			if($es !== false) $ee = strpos($line, $en, $es + 1);
			if($es !== false && $ee !== false && $es < $ee) {
				$buf .= substr($line, 0, $es) . str_replace($de, "\n", substr($line, $es + 1, $ee - $es - 1));
				$line = substr($line, $ee + 1);
			} else {
				$buf .= $line;
				$line = "";
			}
		} while($line != "");
		
		array_push($ret, str_replace("\n", $de, explode($de, $buf)));
	}
	return $ret;
}

// HTTP/1.1プロトコルでアクセスする
function http11($method, $protocol, $host, $port = 0, $page = "/", $query = "", $basic = "", $cookie = "", $autoconv = true, $ua = ENV_PRODUCT_UA, $fr = "", $referer = "") {
	$ret = "";
	$head = "";
	$body = "";
	$chunked = false;
	
	// 接続情報を決定する
	switch($protocol) {
	case "https":
		// $prefix = "tls://";
		$prefix = "ssl://";
		break;
	case "http":
	default:
		$prefix = "";
		break;
	}
	if($port == 0) $port = getservbyname($protocol, "tcp");
	$querystring = ($method == "GET" && $query != "")? "?" . $query: "";
	
	// ソケットを開く
	$context = stream_context_create();
	$result = stream_context_set_option($context, "ssl", "verify_peer", false);
	$result = stream_context_set_option($context, "ssl", "verify_host", false);
	$fp = stream_socket_client($prefix . $host . ":" . (string)$port, $errno, $errstr, 10, STREAM_CLIENT_CONNECT, $context);
	if($fp) {
		fputs($fp, $method . " " . $page . $querystring . " HTTP/1.1\r\n");
		fputs($fp, "Host: " . $host . "\r\n");
		fputs($fp, "Date: " . str_replace("+0000", "GMT", gmdate("r")) . "\r\n");
		fputs($fp, "Accept-Language: ja\r\n");
		fputs($fp, "Cache-Control: no-cache\r\n");
		fputs($fp, "Connection: close\r\n");
		if($ua != "") fputs($fp, "User-Agent: " . $ua . "\r\n");
		if($fr != "") fputs($fp, "From: " . $fr . "\r\n");
		if($referer != "") fputs($fp, "Referer: " . $referer . "\r\n");
		if($basic != "") fputs($fp, "Authorization: Basic " . base64_encode($basic) . "\r\n");
		if($cookie != "") fputs($fp, "Cookie: " . $cookie . "\r\n");
		if($method == "POST") {
			fputs($fp, "Content-Type: application/x-www-form-urlencoded\r\n");
			fputs($fp, "Content-Length: " . strlen($query) . "\r\n");
			fputs($fp, "\r\n" . $query);
		} else {
			fputs($fp, "\r\n");
		}
		
		// Chunkedヘッダーを処理する
		while(!feof($fp)) {
			$line = @fgets($fp, ENV_NUM_HTTP11_SIZE_BUF);
			if(preg_match("/^[Tt][Rr][Aa][Nn][Ss][Ff][Ee][Rr]-[Ee][Nn][Cc][Oo][Dd][Ii][Nn][Gg][\s\t]*:[\s\t]*[Cc][Hh][Uu][Nn][Kk][Ee][Dd]/", $line)) {
				$chunked = true;
				continue;
			}
			$head .= $line;
			if($line == "\r\n") break;
		}
		
		// ボディーを処理する
		if($chunked == true) {
			while(!feof($fp)) {
				$chunk = fgets($fp, ENV_NUM_HTTP11_SIZE_BUF);
				$size = (preg_match("/^([0-9A-Fa-f]+)/", $chunk, $matches))? hexdec($matches[1]): -1;
				if($size <= 0) break;
				$body .= http11_fread_blocking($fp, $size);
				http11_fread_blocking($fp, 2);
			}
		} else {
			while(!feof($fp)) $body .= http11_fread_blocking($fp, ENV_NUM_HTTP11_SIZE_BUF);
		}
		
		// ソケットを閉じる
		fclose($fp);
		
		// HTMLかつUTF-8以外の場合は文字コードを変換する
		$mime = (preg_match("/[Cc][Oo][Nn][Tt][Ee][Nn][Tt]-[Tt][Yy][Pp][Ee][\s\t]*:[\s\t]*([^;\r\n]+)/", $head, $matches) > 0? strtolower($matches[1]): "");
		if($autoconv == true && $mime == "text/html") {
			$metas = parse_tag($body, "meta");
			foreach($metas as $meta) {
				if($meta["charset"] != "") {
					$enc = $meta["charset"];
					break;
				}
				if(strtolower($meta["http-equiv"]) == "content-type" && preg_match("/[Cc][Hh][Aa][Rr][Ss][Ee][Tt][\s\t]*\=[\s\t]*([0-9A-Za-z\-_]+)/", $meta["content"], $matches) > 0) {
					$enc = $matches[1];
					break;
				}
			}
			if($enc == "") $enc = "Shift_JIS";
			if(strtoupper($enc) == "X-SJIS" || strtoupper($enc) == "SHIFT-JIS") $enc = "Shift_JIS";
			switch(strtoupper($enc)) {
			case "UTF-7":
			case "UTF-16":
			case "UTF-32":
			case "EUC-JP":
			case "ISO-2022-JP":
			case "SHIFT_JIS":
			case "SJIS":
				$body = str_replace(array($enc, "−"), array("UTF-8", "－"), mb_convert_encoding($body, "UTF-8", strtoupper($enc)));
				$head = rtrim($head) . "\r\nX-PURSE-Original-Encoding: " . $enc . "\r\n\r\n";
			case "UTF-8":
			default:
				break;
			}
		}
		$ret = $head . $body;
	}
	env_dlog("MTHD: " . $method . "\r\nPROT: " . $protocol . "\r\nHOST: " . $host . "\r\nPORT: " . $port . "\r\nPAGE: " . $page . "\r\nQERY: " . $query . "\r\nBASC: " . $basic . "\r\nCOOK: " . $cookie . "\r\nUAGT: " . $ua . "\r\nFROM: " . $fr . "\r\nREFR: " . $referer . "\r\nMIME: " . $mime . "\r\nENCD: " . $enc . "\r\nHEAD: " . $head . "\r\n\r\n" . $body);
	return $ret;
}

// Chunkedのデータをfreadで読み込みする際、指定サイズ未満しか取得できない場合があるのを改善する
function http11_fread_blocking($fp, $len) {
	$ret = "";
	$size = $len;
	while(!feof($fp) && $size > 0) {
		$buf = fread($fp, $size);
		$size -= strlen($buf);
		$ret .= $buf;
	}
	return $ret;
}

// $memoriesに存在するキーを$headsより探し、$cookieの値を更新する
function update_cookie($memories, $heads, $cookie) {
	$ret = "";
	$cookies = array();
	$kvs = explode(";", $cookie);
	foreach($kvs as $kv) {
		list($k, $v) = explode("=", $kv, 2);
		$k = trim($k);
		$v = trim($v);
		if($k != "" && $v != "") $cookies[$k] = $k . "=" . $v;
	}
	
	foreach($memories as $memory) foreach($heads as $head) {
		$c = preg_match_all("/(" . $memory . ")=([^;\r\n]*)/", $head, $matches);
		for($i = 0; $i < $c; $i++) $cookies[$matches[1][$i]] = $matches[1][$i] . "=" . $matches[2][$i];
	}
	
	$ret = implode("; ", $cookies);
	return $ret;
}

// OFX 1.0.2からOFX 2.1.1へと整形する
function convert_ofx($ofx) {
	$ret = "";
	$head = "";
	$body = "";
	
	// ヘッダーとボディーに分割する
	$i = strpos($ofx, "<OFX>");
	if($i !== false) $head = substr($ofx, 0, $i - 1); else $i = 0;
	$j = strpos($ofx, "</OFX>");
	if($j !== false) $body = substr($ofx, $i, $j + 6);
	
	// ヘッダーを解析する
	$heads = array();
	if(preg_match_all("/([A-Z]+):([^\r\n]+)/", $head, $matches) > 0) for($i = 0; $i < count($matches[0]); $i++) $heads[$matches[1][$i]] = $matches[2][$i];
	if($heads["OFXHEADER"] == "100" && $heads["VERSION"] == "102" && $heads["DATA"] == "OFXSGML") {
		// OFX 1.0.2の場合
		if($heads["ENCODING"] != "" && $heads["ENCODING"] != "UTF-8") $body = mb_convert_encoding($body, "UTF-8", $heads["ENCODING"]);
		$ret .= preg_replace("/<OFX>[\w\W]*<\/OFX>[\r\n]+?/", "", file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_OFX));
		
		// 改行コードを統一する
		$body = str_replace(array("\r\n", "\r"), array("\n", "\n"), $body);
		
		// 行頭のタブを削除し、行頭にないタグを行頭に移動し、空行を削除する
		$body = preg_replace(array("/\n\t+/", "/([^\n])</", "/\n{2,}/"), array("\n", "\\1\n<", "\n"), $body);
		
		$bodys = explode("\n", $body);
		$c = count($bodys);
		for($i = 0; $i < $c; $i++) {
			$bodys[$i] = trim($bodys[$i]);
			if(preg_match("/^<(\/)?([^>]+?)>(.*)$/", $bodys[$i], $matches) > 0) {
				// タグの場合
				if($matches[1] == "/") {
					// 終了タグの場合
					if($i > 0 && strpos($bodys[$i - 1], "<" . $matches[2] . ">") === false) $ret .= $bodys[$i] . "\r\n";
				} else {
					// 開始タグの場合
					$ret .= $bodys[$i] . ($matches[3] != ""? "</" . $matches[2] . ">": "") . "\r\n";
				}
			} else {
				// タグ以外の場合
				$ret .= $bodys[$i] . "\r\n";
			}
		}
	}
	return $ret;
}

// テンプレートよりOFX 2.1.1を生成する
function generate_ofx($status, $str = ENV_STR_OFX_NODATA, $cook = "", $akey = "") {
	$now = ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ;
	$org = ENV_PRODUCT_ID;
	$content = trim($str);
	$code = ENV_STR_OFX_CODE_SUCCESS;
	$serverity = ENV_STR_OFX_SEVERITY_INFO;
	$message = "";
	$mfachallengetrnrs = "";
	$sesscookie = $cook;
	$accesskey = $akey;
	
	switch($status) {
	case ENV_NUM_STATUS_SUCCESS:
		// 何もしない
		break;
	case ENV_NUM_STATUS_ADDITION:
		$code = ENV_STR_OFX_CODE_ADDITION;
		$severity = ENV_STR_OFX_SEVERITY_ERROR;
		$message = "ADDITION";
		$mfachallengetrnrs = $content;
		$content = "";
		break;
	case ENV_NUM_STATUS_MAINTENANCE:
		$code = ENV_STR_OFX_CODE_MAINTENANCE;
		$severity = ENV_STR_OFX_SEVERITY_ERROR;
		$message = "MAINTENANCE";
		break;
	case ENV_NUM_STATUS_CAUTION:
		$code = ENV_STR_OFX_CODE_CAUTION;
		$severity = ENV_STR_OFX_SEVERITY_ERROR;
		$message = "CAUTION";
		break;
	case ENV_NUM_STATUS_FAILURE:
	default:
		$code = ENV_STR_OFX_CODE_FAILURE;
		$severity = ENV_STR_OFX_SEVERITY_ERROR;
		$message = "FAILURE";
		break;
	}
	if($message != "") $message = "<MESSAGE>" . $message . "</MESSAGE>";
	if($sesscookie != "") $sesscookie = "<SESSCOOKIE>" . $sesscookie . "</SESSCOOKIE>";
	if($accesskey != "") $accesskey = "<ACCESSKEY>" . $accesskey . "</ACCESSKEY>";
	
	return str_replace(array("<!--[now]-->", "<!--[org]-->", "<!--[content]-->", "<!--[code]-->", "<!--[severity]-->", "<!--[message]-->", "<!--[mfachallengetrnrs]-->", "<!--[sesscookie]-->", "<!--[accesskey]-->"), array($now, $org, $content, $code, $severity, $message, $mfachallengetrnrs, $sesscookie, $accesskey), file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_OFX));
}

// テンプレートよりHTMLを生成する
function generate_html($settings, $resp) {
	$html = file_get_contents(ENV_FILE_DIR_CLIENT . ENV_FILE_TEMPLATE_HTML);
	$str = "";
	
	switch($resp["status"]) {
	case ENV_NUM_STATUS_ADDITION:
		// 追加認証情報を生成する
		list($text, $attr, $type) = explode("|", $settings["additional"], 3);
		$attr = strtolower($attr);
		switch($attr) {
		case "text":
		case "password":
		case "hidden":
			break;
		default:
			$attr = "text";
			break;
		}
		// breakしない
	case ENV_NUM_STATUS_FAILURE:
	case ENV_NUM_STATUS_MAINTENANCE:
	case ENV_NUM_STATUS_CAUTION:
		$str = file_get_contents(ENV_FILE_DIR_CLIENT . (string)$resp["status"] . ENV_FILE_EXT_HTML);
		break;
	case ENV_NUM_STATUS_SUCCESS:
	case ENV_NUM_STATUS_NONE:
	default:
		// 何もしない
		break;
	}
	
	return str_replace("<!--[content]-->", trim(str_replace(array("<!--[addr]-->", "<!--[fiid]-->", "<!--[name]-->", "<!--[home]-->", "<!--[sid]-->", "<!--[aid]-->", "<!--[sesscookie]-->", "<!--[accesskey]-->", "<!--[additional]-->", "<!--[body]-->", "<!--[attr]-->", "<!--[text]-->"), array($_SERVER["REQUEST_URI"], $settings["fiid"], $settings["name"], $settings["home"], $resp["sid"], $resp["aid"], $resp["sesscookie"], $resp["accesskey"], ($type == "image"? "<img src=\"" . $resp["additional"] . "\" alt=\"画像\" />": htmlspecialchars($resp["additional"], ENT_COMPAT | ENT_HTML401, "UTF-8")), preg_replace("/[\r\n]+/", "<br />\r\n", htmlspecialchars($resp["body"], ENT_COMPAT | ENT_HTML401, "UTF-8")), $attr, $text), $str)), $html);
}

?>
