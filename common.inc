<?php
/*
MoneySound
common.inc: 定数・共通関数を定義する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU All-Permissive License and Beatrek Origin License.
*/

define("ENV_PRODUCT_CODENAME", "PURSE");
define("ENV_PRODUCT_VERSION", "1.0");
define("ENV_PRODUCT_ID", ENV_PRODUCT_CODENAME . "/" . ENV_PRODUCT_VERSION);

define("ENV_FILE_DIR_COMMON", "./common/");

define("ENV_FILE_INI_FI", "fi.ini");
define("ENV_FILE_TEMPLATE_OFX", "ofx.xml");

define("ENV_STR_OFX_BROKER_BANK", "BANK"); // 銀行
define("ENV_STR_OFX_BROKER_CREDITCARD", "CREDITCARD"); // クレジットカード
define("ENV_STR_OFX_BROKER_INVESTMENT", "INVSTMT"); // 証券
define("ENV_STR_OFX_BROKER_PREPAID", "PREPAID"); // 前払式帳票

define("ENV_BOOL_DEBUG", false); // 開発者向け（デバッグ）機能を有効とするか否か

// INIファイルに定義されている金融機関の情報を条件に応じて取り出す
function get_fi_settings($fiid = "") {
	$ret = array();
	$ini = parse_ini_file(ENV_FILE_DIR_COMMON . ENV_FILE_INI_FI, true);
	$ks = array_keys($ini);
	$tag = "";
	foreach($ks as $k) {
		$ini[$k]["fiid"] = $k;
		switch($ini[$k]["type"]) {
		case ENV_STR_OFX_BROKER_BANK:
		case ENV_STR_OFX_BROKER_PREPAID:
			break;
		case ENV_STR_OFX_BROKER_CREDITCARD:
			$tag = "CC";
			break;
		case ENV_STR_OFX_BROKER_INVESTMENT:
			$tag = "INV";
			break;
		default:
			$ini[$k]["type"] = ENV_STR_OFX_BROKER_BANK;
			break;
		}
		$ini[$k]["tag"] = $tag;
	}
	if($fiid != "") {
		if($ini[$fiid]) $ret = $ini[$fiid];
	} else {
		$ret = $ini;
	}
	return $ret;
}

?>
