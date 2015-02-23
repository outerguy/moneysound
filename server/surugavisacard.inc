<?php
/*
surugavisacard.inc: �X���K��s VISA�f�r�b�g/VISA�N���W�b�g��HTML���OFX�t�@�C���𐶐�����
Copyright (C) 2012-2015 Hiromu2000. All Rights Reserved.
mailto:hiromu2000@hotmail.com
Licensed under the GNU AGPLv3.
*/

// ���O�C�������擾����
$user = $ofxforms["loginID"];
$pass1 = $ofxforms["telPin"];
$pass2 = $ofxforms["webPin"];
$mode = $ofxforms["mode"];

$resp = array();
$resp["status"] = ENV_NUM_STATUS_NONE;
$ofx = "";

// ���s���ԁi�^�C���A�E�g�j���Đݒ肷��
@set_time_limit(ENV_NUM_TIMEOUT);

// �z�[����ʂ��擾����
$urls = parse_url($settings["home"]);
$method = "GET";
$protocol = ($urls["scheme"] != ""? $urls["scheme"]: "");
$host = ($urls["host"] != ""? $urls["host"]: "");
$page = ($urls["path"] != ""? $urls["path"]: "/");
$query = "";
$cookie = "";
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

// ���O�C����ʂ��擾����
$protocol = 'https';
$page = '/Top.do';
$cookie = surugavisa_updatecookie($head, $cookie);
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

// ���O�C������
$forms = parse_tag($body, "form");
$c = parse_tag_search($forms, "name", "loginForm");
$method = "POST";
$urls = parse_url($forms[$c]['action']);
if($urls["scheme"] != "") $protocol = $urls["scheme"];
if($urls["host"] != "") $host = $urls["host"];
if($urls["path"] != "") $page = $urls["path"];
if($urls["query"] != "") $page .= "?" . $urls["query"];

$inputs = parse_tag($forms[$c]["innerHTML"], "input");
$queries = array();
foreach ($inputs as $input) {
    if ($input["name"] != "") {
        $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
    }
}
$queries["loginID"] .= $user;
$queries["telPin"] .= $pass1;
$queries["webPin"] .= $pass2;
$query = implode("&", $queries);
$cookie = surugavisa_updatecookie($head, $cookie);
list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);

if(strpos($body, "���������e�i���X���ł�") !== false) {
    // �V�X�e�������e�i���X��ʂ̏ꍇ
    $resp["status"] = ENV_NUM_STATUS_MAINTENANCE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
} else if(strpos($body, "loginForm") !== false) {
    // ���O�C�����s�̏ꍇ
    $resp["status"] = ENV_NUM_STATUS_FAILURE;
    $resp["method"] = $method;
    $resp["uri"] = $uris["scheme"] . "://" . $uris["host"] . $uris["path"];
    $resp["query"] = $query;
    $resp["cookie"] = $cookie;
    $resp["head"] = $head;
    $resp["body"] = $body;
    $resp["ofx"] = generate_ofx($resp["status"]);
} else {
    // �J�[�h���́E�J�[�h�ԍ����擾����
    $account['acctname'] = '�X���K Visa�f�r�b�g/�N���W�b�g';
    $account['acctid'] = substr($user, 12);
    // DOM�c���[�𐶐�
    $ofxdom = new ofxDOM($account['acctid']);
    
    // ���j���[����u�����p���ׁv��I��
    $cds = array();
    for ($i=0;$i<6;$i++) {
        $forms = parse_tag($body, "form");
        $c = parse_tag_search($forms, "name", "meisaiForm");
        $page = '/meisai.do';
        $inputs = parse_tag($forms[$c]["innerHTML"], "input");
        $queries = array();
        foreach ($inputs as $input) {
            if ($input["name"] != "") {
                $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
            }
        }
        $queries["month"] = "month=" . $i;
        $query = implode("&", $queries);
        $cookie = surugavisa_updatecookie($head, $cookie);
        list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
        // ���ׂ��p�[�X
        $cds = array_merge($cds, surugavisa_parsedom($body));
    }
    usort($cds, function($a, $b) {
        return $a['DTPOSTED'] > $b['DTPOSTED'];
    });
    $ofxdom->setTrans($cds);
    $cds_balamt = 0;
    $cds_s = "";
    $cds_e = "";
    $items = $ofxdom->getTrans();
    foreach ($items as $item) {
        $dtposted = $item->getElementsByTagName('DTPOSTED')->item(0)->nodeValue;
        // DTSTART, DTEND���擾
        if($cds_s == "") $cds_s = $dtposted;
        $cds_e = $dtposted;
        // �c�����v�Z
        $cds_balamt += (double)$item->getElementsByTagName('TRNAMT')->item(0)->nodeValue;
    }
    
    // BANKTRANLIST
    $ofxdom->setBankTranList($cds_s, $cds_e);
    
    // �c��������
    $cds_balamt = (-1)*$cds_balamt;
    $ofxdom->setLedgerBalance($cds_balamt, ENV_STR_DATE_TODAY . ENV_STR_OFX_TZ);
    
    //MKGTINFO��ݒ�
    $ofxdom->setMktginfo(mb_convert_string($settings["name"]));
    
    // XML DOM�c���[�𕶎���ɕϊ�
    $xml = $ofxdom->getXML();
    
    $resp["status"] = ENV_NUM_STATUS_SUCCESS;
    $resp["ofx"] = generate_ofx($mode, $xml);
    
    // ���s���ԁi�^�C���A�E�g�j���Đݒ肷��
    @set_time_limit(ENV_NUM_TIMEOUT);
    
    // ���O�A�E�g����
    $forms = parse_tag($body, "form");
    $c = parse_tag_search($forms, "name", "logoutForm");
    $page = '/logout.do';
    $inputs = parse_tag($forms[$c]["innerHTML"], "input");
    $queries = array();
    foreach ($inputs as $input) {
        if ($input["name"] != "") {
            $queries[$input["name"]] = urlencode($input["name"]) . "=" . urlencode($input["value"]);
        }
    }
    $query = implode("&", $queries);
    $cookie = surugavisa_updatecookie($head, $cookie);
    list($head, $body) = surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
}
return $resp;

// HTTP/1.1
function surugavisa_http11($method, $protocol, $host, $page = "/", $query = "", $cookie = "") {
    $ret = "INVALID HOST";
    if(preg_match("/card\.surugabank\.co\.jp$/", $host) > 0) $ret = http11($method, $protocol, $host, 0, $page, $query, "", $cookie);
    if(preg_match("/[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][\s\t]*:[\s\t]*([^\r\n]+)/", $ret, $matches) > 0) {
        $urls = parse_url($matches[1]);    
        if($urls["scheme"] != "") $protocol = $urls["scheme"];
        if($urls["host"] != "") $host = $urls["host"];
        if($urls["path"] != "") $page = $urls["path"];
        $query = ($urls["query"] != ""? $urls["query"]: "");
        return surugavisa_http11($method, $protocol, $host, $page, $query, $cookie);
    }
    return explode("\r\n\r\n", $ret, 2);
}

function surugavisa_updatecookie($head, $cookie) {
    return update_cookie(array("JSESSIONID"), parse_header($head, "set-cookie"), $cookie);
}

function surugavisa_parsedom($str) {
    // ���ו\�ǂݍ��ݗpDOM�c���[�쐬
    $doc = new DOMDocument();
    // <meta �c charset=Shift_JIS">�̑O��<title>���C���������̌����ƂȂ邽�߁C�폜
    preg_match("/(<body.*<\/body>)/s", $str, $matches);
    $str = $matches[1];
    // �w�b�_�t��   
    $str = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><\/head>" . $str . "<\/html>";
    // �����G���R�[�h�ϊ�
    $str = mb_convert_encoding($str, 'UTF-8', 'SJIS');
    // &nbsp;
    $str = str_replace("&nbsp;", "", $str);
    // HTML����DOM�c���[�쐬
    @$doc->loadHTML($str);
    $xpath = new DOMXPath($doc);
    // ���ו\�̃e�[�u�����w��
    $rows = $xpath->query("//table[@class='table-type2 clear margin-b-10px']/tr"); 
    
    $ret = "";
    $cds = array();
    $nrow = $rows->length;
    
    for($i=0; $i<$nrow; $i++) {
        $cd = array();
        $row = $rows->item($i);
        $cols1 = $row->getElementsByTagName('th');
        $cols2 = $row->getElementsByTagName('td');
        // ���p���ׂłȂ��s�̓X�L�b�v
        if ($cols1->length != 2) continue;
        
        $cd["TRNTYPE"] = ENV_STR_OFX_TRNTYPE_CREDIT;
        
        // ���t���擾����
        preg_match("/([0-9]{4}).*?([0-9]{1,2}).*?([0-9]{1,2})/", str_replace(" ", "", $cols1->item(1)->nodeValue), $matches);
        $cd["DTPOSTED"] = sprintf("%d%02d%02d", $matches[1], $matches[2], $matches[3]);
        
        // �g�����U�N�V�����ԍ��i�������ƃf�[�^��ʁj�𐶐�����
        $cd["FITID"] = '000';
        // �E�v���擾����
        $cd["NAME"] = $cols2->item(0)->nodeValue;
        $cd["NAME"] = preg_replace('/-/', mb_convert_encoding('�[', 'UTF-8', 'sjis-win'), $cd["NAME"]);
        // ���z���擾����
        $cd["TRNAMT"] = (-1)*(double)parse_amount(trim($cols2->item(1)->nodeValue));
        
        array_push($cds, $cd);
    }
    return $cds;

}
?>