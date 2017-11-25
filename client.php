<?php
/*
client.php: JavaScriptを生成する
Copyright (C) 2012-2017 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the Apache License 2.0 and Beatrek Origin License.
*/

require_once("./common.inc");
require_once("./client.inc");

// JavaScriptテンプレートを読み込む
$js = file_get_contents(ENV_FILE_DIR_CLIENT . ENV_FILE_TEMPLATE_JS);

// OFXテンプレートを読み込む
$ofx = file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_OFX);

// PDFテンプレートを読み込む
$pdf = file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_PDF);

// OFXを文字列形式に変換する
$ofxhead = addcslashes(trim(preg_replace("/<OFX>[\w\W]*<\/OFX>[\r\n]+?/", "", $ofx)), "\"\r\n") . "\\r\\n";

// PDFを文字列形式に変換する
$pdftext = addcslashes($pdf, "\"\r\n");

// XMLファイルに定義されている金融機関をJSON形式に変換する
$fis = get_fi_settings();
$filist = trim(json_encode($fis, JSON_UNESCAPED_UNICODE + JSON_UNESCAPED_SLASHES));

// 埋め込み文字列を置換する
$js = str_replace(array("<!--[client]-->", "<!--[server]-->", "<!--[family]-->", "<!--[ofxhead]-->", "<!--[pdftext]-->", "\"<!--[filist]-->\"", "\"<!--[debug]-->\""), array(ENV_PRODUCT_CLIENT_VERSION, ENV_PRODUCT_VERSION, ENV_PRODUCT_FAMILY, $ofxhead, $pdftext, $filist, (ENV_BOOL_DEBUG == true? "true": "false")), $js);

// レスポンスを返す
header(get_http_status(ENV_NUM_STATUS_SUCCESS));
header("Cache-Control: no-cache");
header("Pragma: no-cache");
header("Content-Type: text/javascript; charset=UTF-8");
header("Content-Length: " . strlen($js));
echo $js;

?>
