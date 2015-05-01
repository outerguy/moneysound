<?php
/*
MoneySound
server.php: �F�؏����󂯎���ċ��Z�@�֖��̏��������s����
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the GNU AGPLv3 and Beatrek Origin License.
*/

require_once("./common.inc");
require_once("./server.inc");

$resp = array();
$ofxforms = array();
$debug = "";

// �F�؏����󂯎��
while(list($k, $v) = each($_POST)) {
	$ofxforms[$k] = parse_param($v);
	$debug .= $k . "=" . $v . "\r\n";
}
$ofxforms["fiid"] = basename($ofxforms["fiid"]);
env_dlog($debug);

// ���Z�@�֖��̏��������s����
if(file_exists(ENV_FILE_DIR_SERVER . $ofxforms["fiid"] . ENV_FILE_EXT_INC) == true && is_readable(ENV_FILE_DIR_SERVER . $ofxforms["fiid"] . ENV_FILE_EXT_INC) == true) {
	$settings = get_fi_settings($ofxforms["fiid"]);
	$resp = require_once(ENV_FILE_DIR_SERVER . $ofxforms["fiid"] . ENV_FILE_EXT_INC);
}

// ���g�����݂��Ȃ��ꍇ�A�����o�͂��Ȃ�
if($resp["ofx"] == "") $resp["status"] = ENV_NUM_STATUS_NONE;

// ���X�|���X��Ԃ�
header(get_http_status($resp["status"]));
header("Cache-Control: no-cache");
header("Pragma: no-cache");

if($resp["token"] != "") header("X-Token: " . $resp["token"]);

switch($resp["status"]) {
case ENV_NUM_STATUS_SUCCESS:
case ENV_NUM_STATUS_ADDITION:
case ENV_NUM_STATUS_FAILURE:
case ENV_NUM_STATUS_MAINTENANCE:
case ENV_NUM_STATUS_CAUTION:
	header("Content-Type: application/x-ofx; charset=UTF-8");
	header("Content-Disposition: attachment; filename=\"" . $settings["fiid"] . "_" . ENV_STR_DATE_TODAY . ENV_FILE_EXT_OFX . "\"");
	break;
case ENV_NUM_STATUS_NONE:
default:
	break;
}

$n = strlen($resp["ofx"]);
if($n > 0) {
	header("Content-Length: " . (string)$n);
	echo $resp["ofx"];
}

?>
