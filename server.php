<?php
/*
server.php: 認証情報を受け取って金融機関毎の処理を実行する
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU AGPLv3 and Beatrek Origin License.
*/

require_once("./common.inc");
require_once("./server.inc");

$resp = array();
$ofxforms = array();
$debug = "";

// 認証情報を受け取る
while(list($k, $v) = each($_POST)) {
	$ofxforms[$k] = parse_param($v);
	$debug .= $k . "=" . $v . "\r\n";
}
$ofxforms["fiid"] = basename($ofxforms["fiid"]);
env_dlog($debug);

// 金融機関毎の処理を実行する
$inc = ENV_FILE_DIR_SERVER . $ofxforms["fiid"] . ENV_FILE_EXT_INC;
if(file_exists($inc) == true && is_readable($inc) == true) {
	$settings = get_fi_settings($ofxforms["fiid"]);
	$resp = require_once($inc);
}

// 中身が存在しない場合、何も出力しない
if($resp["ofx"] == "") $resp["status"] = ENV_NUM_STATUS_NONE;

// 中身を取り出す
$content = $resp["ofx"];

// ヘッダーを出力する
header(get_http_status($resp["status"]));
header("Cache-Control: no-cache");
header("Pragma: no-cache");

// トークンが存在する場合、X-Tokenヘッダーを出力する
if($resp["token"] != "") header("X-Token: " . $resp["token"]);

switch($resp["status"]) {
case ENV_NUM_STATUS_SUCCESS:
case ENV_NUM_STATUS_ADDITION:
case ENV_NUM_STATUS_FAILURE:
case ENV_NUM_STATUS_MAINTENANCE:
case ENV_NUM_STATUS_CAUTION:
	// 正常・エラーの場合、OFXファイルを出力する
	header("Content-Type: application/x-ofx; charset=UTF-8");
	header("Content-Disposition: attachment; filename=\"" . $settings["fiid"] . "_" . ENV_STR_DATE_TODAY . ENV_FILE_EXT_OFX . "\"");
	break;
case ENV_NUM_STATUS_NONE:
default:
	// 空、またはその他の場合、何も出力しない
	$content = "";
	break;
}

// ボディーを出力する
$n = strlen($content);
if($n > 0) {
	header("Content-Length: " . (string)$n);
	echo $content;
}

?>
