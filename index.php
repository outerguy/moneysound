<?php
/*
MoneySound
index.php: HTMLを生成する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU All-Permissive License and Beatrek Origin License.
*/

require_once("./client.inc");

// HTMLを取得する
$html = file_get_contents(ENV_FILE_DIR_CLIENT . ENV_FILE_TEMPLATE_HTML);

// レスポンスを返す
header("HTTP/1.0 200 OK");
header("Cache-Control: no-cache");
header("Pragma: no-cache");
header("Content-Type: text/html; charset=UTF-8");
header("Content-Length: " . strlen($html));
echo $html;

?>
