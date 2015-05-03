<?php
/*
MoneySound
common.inc: 定数・共通関数を定義する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU All-Permissive License and Beatrek Origin License.
*/

define("ENV_PRODUCT_CODENAME", "PURSE");
define("ENV_PRODUCT_VERSION", "1.1");
define("ENV_PRODUCT_ID", ENV_PRODUCT_CODENAME . "/" . ENV_PRODUCT_VERSION);

define("ENV_FILE_DIR_COMMON", "./common/");

define("ENV_FILE_TEMPLATE_FI", "fi.xml");
define("ENV_FILE_TEMPLATE_OFX", "ofx.xml");
define("ENV_FILE_TEMPLATE_PDF", "pdf.dat");

define("ENV_STR_OFX_BROKER_BANK", "BANK"); // 銀行
define("ENV_STR_OFX_BROKER_CREDITCARD", "CREDITCARD"); // クレジットカード
define("ENV_STR_OFX_BROKER_INVESTMENT", "INVSTMT"); // 証券
define("ENV_STR_OFX_BROKER_PREPAID", "PREPAID"); // 前払式帳票

define("ENV_BOOL_DEBUG", false); // 開発者向け（デバッグ）機能を有効とするか否か

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
		
		$inputs = array();
		foreach($fi->form->input as $input) {
			array_push($inputs, mb_htmlspecialchars((string)$input["id"]));
			$settings[$id][(string)$input["id"]] = mb_htmlspecialchars(implode("|", array((string)$input, (string)$input["type"])));
		}
		$settings[$id]["form"] = implode("|", $inputs);
		
		if(isset($fi->itemlist) == true) foreach($fi->itemlist->item as $item) {
			$settings[$id][(string)$item["id"]] = mb_htmlspecialchars((string)$item);
		}
	}
	
	if($fiid != "") {
		if(isset($settings[$fiid]) == true) $ret = $settings[$fiid];
	} else {
		$ret = $settings;
	}
	return $ret;
}

function mb_htmlspecialchars($str) {
	return htmlspecialchars($str, ENT_COMPAT | ENT_XML1, "UTF-8");
}

?>
