<?php
/*
MoneySound
client.php: JavaScript�𐶐�����
Copyright (C) 2012-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the Apache License 2.0 and Beatrek Origin License.
*/

require_once("./common.inc");
require_once("./client.inc");

// OFX�w�b�_�𕶎���`���ɕϊ����Ė��ߍ���
$ofxhead = addcslashes(trim(preg_replace("/<OFX>[\w\W]*<\/OFX>[\r\n]+?/", "", file_get_contents(ENV_FILE_DIR_COMMON . ENV_FILE_TEMPLATE_OFX))), "\"\r\n") . "\\r\\n";

// INI�t�@�C���ɒ�`����Ă�����Z�@�ւ�JSON�`���ɕϊ����Ė��ߍ���
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

// ���ߍ��ݕ������u������
$js = str_replace(array("<!--[version]-->", "<!--[ofxhead]-->", "\"<!--[filist]-->\"", "\"<!--[debug]-->\""), array(ENV_PRODUCT_FAMILY_VERSION, $ofxhead, $filist, (ENV_BOOL_DEBUG == true? "true": "false")), file_get_contents(ENV_FILE_DIR_CLIENT . ENV_FILE_TEMPLATE_JS));

// ���X�|���X��Ԃ�
header("HTTP/1.0 200 OK");
header("Cache-Control: no-cache");
header("Pragma: no-cache");
header("Content-Type: text/javascript; charset=UTF-8");
header("Content-Length: " . strlen($js));
echo $js;

?>
