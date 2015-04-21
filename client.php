<?php
/*
MoneySound
client.php: JavaScriptを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the Apache License 2.0 and Beatrek Origin License.
*/

require_once("./common.inc");
require_once("./client.inc");

// OFXヘッダを文字列形式に変換して埋め込む
$ofxhead = addcslashes(trim(preg_replace("/<OFX>[\w\W]*<\/OFX>[\r\n]+?/", "", file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_OFX))), "\"\r\n") . "\\r\\n";

// INIファイルに定義されている金融機関をJSON形式に変換して埋め込む
$fis = get_fi_settings();

$filists = array();
$i = 0;
foreach($fis as $mk => $mv) {
	foreach($mv as $k => $v) {
		$filists[$mk][$k] = mb_convert_encoding($v, "UTF-8", "Shift_JIS");
	}
	$i++;
}
$filist = trim(json_encode($filists, JSON_UNESCAPED_UNICODE + JSON_UNESCAPED_SLASHES));

// 埋め込み文字列を置換する
$js = str_replace(array("<!--[family]-->", "<!--[purse]-->", "<!--[ofxhead]-->", "\"<!--[filist]-->\"", "\"<!--[debug]-->\""), array(ENV_PRODUCT_FAMILY_VERSION, ENV_PRODUCT_VERSION, $ofxhead, $filist, (ENV_BOOL_DEBUG == true? "true": "false")), file_get_contents(ENV_FILE_DIR_CLIENT . ENV_FILE_TEMPLATE_JS));

// レスポンスを返す
header("HTTP/1.0 200 OK");
header("Cache-Control: no-cache");
header("Pragma: no-cache");
header("Content-Type: text/javascript; charset=UTF-8");
header("Content-Length: " . strlen($js));
echo $js;

?>
