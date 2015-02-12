/*
MoneySound
main.js: 画面を制御する
Copyright (C) 2014-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the Apache License 2.0 and Beatrek Origin License.
*/

var rel = "3.0";
var get_all = -1;
var xhr;

var debug = "<!--[debug]-->";
var ofxhead = "<!--[ofxhead]-->";
var fiids = "<!--[filist]-->";
var ficats = new Array();
var filists = new Array();

fiids["local"] = { "type": "LOCAL", "name": "ログオン", "form": "localid,localpass", "localid": "ローカルID,text", "localpass": "ローカルパスワード,password" };
fiids["create"] = { "type": "LOCAL", "name": "登録", "form": "localid,localpass", "localid": "ローカルID,text", "localpass": "ローカルパスワード,password" };
fiids["delete"] = { "type": "LOCAL", "name": "抹消", "form": "localid", "localid": "ローカルID,text" };
ficats["BANK"] = "銀行";
ficats["CREDITCARD"] = "クレジットカード";
ficats["INVSTMT"] = "証券";
ficats["PREPAID"] = "前払式帳票";
for(i in ficats) filists[i] = new Array();
for(i in fiids) if(typeof filists[fiids[i]["type"]] != "undefined") filists[fiids[i]["type"]][i] = fiids[i];

var themes = new Array();
themes["standard.css"] = "標準（スマートフォン対応）";
themes["modern.css"] = "Modern";
themes["aero.css"] = "Aero";
themes["luna.css"] = "Luna";
themes["aqua.css"] = "Aqua";
themes["precious.css"] = "プレシャス";

(function() {
	self.window.onload = fnc_load;
	self.document.onkeydown = function(e) {
		var ret = true;
		if(typeof e == "undefined") e = event;
		if(e != null) switch(e.keyCode) {
		case 27: // esc
			if(xhr != null && xhr.readyState != 4) {
				dom_get_id("root").reset();
				ret = false;
			}
			break;
		}
		return ret;
	};
})();


// =========================================================================
// 機能
// =========================================================================

// 事前処理機能
function fnc_load() {
	var tag_root = dom_get_id("root");
	var tag_as = dom_get_tag("a");
	var tag_p;
	var i;
	
	// デバッグ機能が有効の場合、警告を表示する
	if(debug != false) {
		tag_p = dom_create_tag("p", { "class": "ac", "style": "padding: 0.5em; font-weight: bold; color: #FFFFFF; background: #FF0000;" });
		tag_p.appendChild(dom_create_text("【警告】開発者向け（デバッグ）機能が有効のため、認証情報を含む詳細な記録が残ります。開発者以外の方は、操作しないでください。または、開発者へご相談ください。"));
		tag_root.parentNode.insertBefore(tag_p, tag_root);
		dom_get_id("btn_debug").style.display = "inline";
	}
	
	// ボタンと機能を関連付ける
	tag_root.onreset = fnc_cancel;
	with(dom_get_id("btn_logon")) onclick = onkeypress = fnc_logon;
	with(dom_get_id("btn_logoff")) onclick = onkeypress = fnc_logoff;
	with(dom_get_id("btn_register")) onclick = onkeypress = fnc_register;
	with(dom_get_id("btn_erase")) onclick = onkeypress = fnc_erase;
	with(dom_get_id("btn_debug")) onclick = onkeypress = fnc_debug;
	with(dom_get_id("btn_option")) onclick = onkeypress = fnc_option;
	with(dom_get_id("btn_version")) onclick = onkeypress = fnc_version;
	with(dom_get_id("btn_get_all")) onclick = onkeypress = fnc_update_all;
	with(dom_get_id("btn_ofx_all")) onclick = onkeypress = fnc_ofx_all;
	with(dom_get_id("btn_add")) onclick = onkeypress = fnc_create;
	
	// リンク先を設定する
	for(i = 0; i < tag_as.length; i++) tag_as[i].target = "link";
	
	fnc_initialize();
	
	return true;
}

// 初期化機能
function fnc_initialize() {
	var ret = false;
	var tag_table = dom_get_id("account");
	var tag_p;
	var logons, lists;
	var i;
	
	if(chkenv_run() == false) {
		tag_p = dom_create_tag("p", { "class": "ac" });
		tag_p.appendChild(dom_create_text("ご利用のブラウザーでは実行できません。"));
		
		tag_table.parentNode.replaceChild(tag_p, tag_table);
		
		dom_get_id("btn_logon").disabled = true;
		dom_get_id("btn_logoff").disabled = true;
		dom_get_id("btn_register").disabled = true;
		dom_get_id("btn_erase").disabled = true;
		dom_get_id("btn_debug").disabled = true;
		dom_get_id("btn_option").disabled = true;
		dom_get_id("btn_version").disabled = false;
	} else {
		logons = local_current();
		lists = dom_get_storage(logons["localid"], logons["localpass"]);
		switch(lists) {
		case null:
		case "":
			// ログオン情報を削除する
			for(i in logons) dom_del_storage(i);
			
			if(dom_get_id("logon")) dom_get_id("logon").firstChild.nodeValue = "ログオンしてください";
			
			dom_get_id("btn_logon").disabled = false;
			dom_get_id("btn_logoff").disabled = true;
			dom_get_id("btn_register").disabled = false;
			dom_get_id("btn_erase").disabled = false;
			dom_get_id("btn_debug").disabled = true;
			dom_get_id("btn_get_stop").disabled = true;
			dom_get_id("btn_add").disabled = true;
			dom_get_id("btn_option").disabled = true;
			
			dom_get_id("btn_get_all").disabled = true;
			dom_get_id("btn_ofx_all").disabled = true;
			
			lists = "";
			break;
		default:
			dom_get_id("logon").firstChild.nodeValue = logons["localid"];
			
			dom_get_id("btn_logon").disabled = true;
			dom_get_id("btn_logoff").disabled = false;
			dom_get_id("btn_register").disabled = true;
			dom_get_id("btn_erase").disabled = true;
			dom_get_id("btn_debug").disabled = false;
			dom_get_id("btn_get_stop").disabled = true;
			dom_get_id("btn_add").disabled = false;
			dom_get_id("btn_option").disabled = false;
			
			dom_get_id("btn_get_all").disabled = true;
			dom_get_id("btn_ofx_all").disabled = true;
			
			ret = true;
			break;
		}
		
		fnc_option_change();
		
		// 口座一覧を生成する
		fnc_listall(lists.split("\r\n"));
	}
	
	return ret;
}

// ログオン機能
function fnc_logon() {
	var fiid = "local";
	var body = document.createDocumentFragment();
	var auths = new Array();
	var lists = new Array();
	var f = false;
	var tag_p;
	var inputs, dec;
	var i, j;
	
	if(dom_get_id("modal") == null) {
		if(typeof auth != "undefined") {
			lists = auth.split("\t");
			for(i = 0; i < lists.length; i++) {
				j = lists[i].indexOf("=");
				if(j != -1) auths[lists[i].substring(0, j)] = lists[i].substring(j + 1);
			}
		}
		
		// 入力項目を設定する
		inputs = fiids[fiid]["form"].split(",");
		for(i = 0; i < inputs.length; i++) {
			lists = fiids[fiid][inputs[i]].split(",", 2);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(lists[0]));
			body.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "value": (typeof auths[inputs[i]] == "string"? auths[inputs[i]]: "") , "class": "ipt", "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			body.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show(fiids[fiid]["name"], body, true, inputs[0]);
		
		// ログオン画面の未入力項目をチェックする
		form_empty_check();
	} else {
		// コールバックの場合
		if(dom_get_id("fiid") == null) {
			// エラー画面を表示した後の場合、ログオン画面を表示する
			modal_hide();
			fnc_logon();
		} else {
			// ログオン情報を生成する
			fiid = dom_get_id("fiid").value;
			inputs = fiids[fiid]["form"].split(",");
			auths.push("=" + fiid);
			for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
			modal_hide();
			
			// 暗号化データを取得する
			dec = dom_get_storage(auths[1].split("=", 2)[1], auths[2].split("=", 2)[1]);
			switch(dec) {
			case null:
				// 暗号化データが存在しない場合、エラー画面を表示する
				modal_show("エラー", "正しい" + fiids["local"][auths[1].split("=", 2)[0]].split(",", 2)[0] + "を入力してください。", false);
				break;
			case "":
				// 正しく復号できない場合、エラー画面を表示する
				modal_show("エラー", "正しい" + fiids["local"][auths[2].split("=", 2)[0]].split(",", 2)[0] + "を入力してください。", false);
				break;
			default:
				// ログオン情報を設定する
				dom_set_storage(auths[1].split("=", 2)[0], auths[1].split("=", 2)[1]);
				dom_set_storage(auths[2].split("=", 2)[0], auths[2].split("=", 2)[1]);
				
				fnc_initialize();
				
				break;
			}
		}
	}
	return false;
}

// ログオフ機能
function fnc_logoff() {
	var logons = local_current();
	var i;
	
	// ログオン情報を削除する
	for(i in logons) dom_del_storage(i);
	
	fnc_initialize();
	
	return false;
}

// 登録機能
function fnc_register() {
	var fiid = "create";
	var body = document.createDocumentFragment();
	var auths = new Array();
	var tag_p;
	var inputs;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 入力項目を設定する
		inputs = fiids[fiid]["form"].split(",");
		for(i = 0; i < inputs.length; i++) {
			lists = fiids[fiid][inputs[i]].split(",", 2);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(lists[0]));
			body.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "class": "ipt", "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			body.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show(fiids[fiid]["name"], body, true, inputs[0]);
		
		// 登録画面の未入力項目をチェックする
		form_empty_check();
	} else {
		// コールバックの場合
		if(dom_get_id("fiid") == null) {
			// エラー画面を表示した後の場合、登録画面を表示する
			modal_hide();
			fnc_register();
		} else {
			// ログオン情報を生成する
			fiid = dom_get_id("fiid").value;
			inputs = fiids[fiid]["form"].split(",");
			auths.push("=" + fiid);
			for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
			modal_hide();
			
			switch(dom_get_storage(auths[1].split("=", 2)[1], auths[2].split("=", 2)[1])) {
			case null:
				// ログオン情報を設定する
				dom_set_storage(auths[1].split("=", 2)[1], "", auths[2].split("=", 2)[1]);
				modal_showonly("完了", auths[1].split("=", 2)[1] + "を登録しました。ログオンしてください。", false);
				break;
			case "":
			default:
				modal_show("エラー", auths[1].split("=", 2)[1] + "は既に存在しています。", false);
				break;
			}
		}
	}
	return false;
}

// 抹消機能
function fnc_erase() {
	var fiid = "delete";
	var body = document.createDocumentFragment();
	var auths = new Array();
	var tag_p, tag_label;
	var input, key;
	if(dom_get_id("modal") == null) {
		// 入力項目を設定する
		input = fiids[fiid]["form"].split(",")[0];
		lists = fiids[fiid][input].split(",", 2);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text(lists[0]));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": input, "id": input, "class": "ipt", "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("確認"));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "checkbox", "name": "confirm", "id": "confirm", "value": "", "onclick": "form_empty_check();", "onkeydown": "this.onclick();" }));
		tag_label = dom_create_tag("label", { "for": "confirm" });
		tag_label.appendChild(dom_create_text("抹消する（元に戻せません）"));
		tag_p.appendChild(tag_label);
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show(fiids[fiid]["name"], body, true, input);
		
		// 抹消画面の未入力項目をチェックする
		form_empty_check();
	} else {
		// コールバックの場合
		if(dom_get_id("fiid") == null) {
			// エラー画面を表示した後の場合、抹消画面を表示する
			modal_hide();
			fnc_erase();
		} else {
			// ログオン情報を生成する
			fiid = dom_get_id("fiid").value;
			input = fiids[fiid]["form"].split(",")[0];
			key = dom_get_id(input).value;
			modal_hide();
			switch(dom_get_storage(key, "")) {
			case null:
				modal_show("エラー", key + "は存在しません。", false);
				break;
			case "":
			default:
				// ログオン情報、およびOFXを削除する
				dom_del_storage(key, "*");
				modal_showonly("完了", key + "を抹消しました。", false);
				break;
			}
		}
	}
	return false;
}

// デバッグ情報機能
function fnc_debug() {
	var logons = local_current();
	var auths = dom_get_storage(logons["localid"], logons["localpass"]).split("\r\n");
	
	var body = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var fiid;
	for(i = 0; i < auths.length; i++) {
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text(auths[i]));
		body.appendChild(tag_p);
	}
	
	// ダイアログを開く
	modal_showonly("デバッグ情報", body, false);
	return false;
}

// 設定機能
function fnc_option() {
	var logons = local_current();
	var body = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var css;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 画面のテーマリストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("画面のテーマ"));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "theme", "id": "theme", "class": "ipt" });
		for(i in themes) {
			tag_option = dom_create_tag("option", { "value": i });
			if(dom_get_id("css_theme").href.indexOf(i) != -1) tag_option["selected"] = "selected";
			tag_option.appendChild(dom_create_text(themes[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show("設定", body, true, "theme");
		
		// 分類リストの先頭を選択する
		tag_select.focus();
	} else {
		// コールバックの場合
		css = dom_get_id("theme")[dom_get_id("theme").selectedIndex].value;
		
		// ダイアログを閉じる
		modal_hide();
		
		dom_set_storage(logons["localid"] + ":theme", css, logons["localpass"]);
		fnc_option_change();
	}
	return false;
}

// 設定を変更する
function fnc_option_change() {
	var logons = local_current();
	var css = dom_get_storage(logons["localid"] + ":theme", logons["localpass"]);
	var i;
	
	if(css == null || css == "") for(i in themes) {
		css = i;
		break;
	}
	with(dom_get_id("css_theme")) href = href.substring(0, href.lastIndexOf("/") + 1) + css;
}

// バージョン情報機能
function fnc_version() {
	var body = document.createDocumentFragment();
	var title = dom_get_id("title").firstChild.nodeValue;
	var tag_p, tag_a, tag_img;
	
	if(dom_get_id("modal") == null) {
		tag_p = dom_create_tag("p");
		tag_img = dom_create_tag("img", { "src": "./client/wsofx.gif", "width": "88", "height": "31", "alt": "We Support OFX", "style": "float: right; clear: right;" });
		tag_p.appendChild(tag_img);
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_img = dom_create_tag("img", { "src": "./client/icon.gif", "width": "32", "height": "32", "alt": title + "アイコン", "style": "float: left; clear: left; margin-right: 1em;" });
		tag_p.appendChild(tag_img);
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "style": "margin-bottom: 8px; line-height: 32px; font-weight: bold;" });
		tag_a = dom_create_tag("a", { "href": "https://github.com/outerguy/moneysound/", "target": "_blank", "style": "margin-right: 0.5em;" });
		tag_a.appendChild(dom_create_text(title));
		tag_p.appendChild(tag_a);
		tag_p.appendChild(dom_create_text("R" + rel));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text("Copyright &copy; 2008-2015 OFFICE OUTERGUY. All rights reserved."));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text("Portions Copyright &copy; 2012-2015 Hiromu Takahashi. All rights reserved."));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("暗号化ライブラリー"));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_a = dom_create_tag("a", { "href": "http://code.google.com/p/crypto-js/", "target": "_blank", "style": "margin-right: 0.5em;" });
		tag_a.appendChild(dom_create_text("CryptoJS v3.1.2"));
		tag_p.appendChild(tag_a);
		tag_p.appendChild(dom_create_text("(c) 2009-2013 by Jeff Mott. All rights reserved."));
		body.appendChild(tag_p);
		
		if(chkenv_xmlhttprequest() == false || chkenv_webstorage() == false || chkenv_domparser() == false || chkenv_xmlserializer() == false || chkenv_blob() == false || chkenv_createobjecturl() == false) {
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text("ご利用のブラウザーが対応していない機能"));
			body.appendChild(tag_p);
			
			if(chkenv_xmlhttprequest() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("XMLHttpRequest"));
				body.appendChild(tag_p);
			}
			if(chkenv_webstorage() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("WebStorage"));
				body.appendChild(tag_p);
			}
			if(chkenv_domparser() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("DOMParser"));
				body.appendChild(tag_p);
			}
			if(chkenv_xmlserializer() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("XMLSerializer"));
				body.appendChild(tag_p);
			}
			if(chkenv_blob() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("Blob"));
				body.appendChild(tag_p);
			}
			if(chkenv_createobjecturl() == false) {
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text("createObjectURL"));
				body.appendChild(tag_p);
			}
		}
		
		// ダイアログを開く
		modal_show("バージョン情報", body, false);
	} else {
		// ダイアログを閉じる
		modal_hide();
	}
	return false;
}

// 追加機能
function fnc_create() {
	var body = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var fiid;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 分類リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("分類"));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "ficat", "id": "ficat", "class": "ipt", "onchange": "fnc_create_change(this[this.selectedIndex].value);", "onkeyup": "this.onchange();" });
		for(i in ficats) {
			tag_option = dom_create_tag("option", { "value": i });
			tag_option.appendChild(dom_create_text(ficats[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		body.appendChild(tag_p);
		
		// 金融機関リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("金融機関"));
		body.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "fiid", "id": "fiid", "size": "8", "class": "ipt", "ondblclick": "dom_get_id(\"modal\").onsubmit();" });
		tag_p.appendChild(tag_select);
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show("追加", body, true, "ficat");
		
		// 分類リストの先頭を選択する
		fnc_create_change();
	} else {
		// コールバックの場合
		fiid = dom_get_id("fiid").options[dom_get_id("fiid").selectedIndex].value;
		
		// ダイアログを閉じる
		modal_hide();
		
		// 変更画面を表示する
		fnc_modify("=" + fiid);
	}
	return false;
}

// 分類を変更した場合に金融機関リストを更新する
function fnc_create_change(cat) {
	var lists = filists[(typeof cat == "undefined"? dom_get_id("ficat")[dom_get_id("ficat").selectedIndex].value: cat)];
	var i;
	
	// 金融機関リストを削除する
	with(dom_get_id("fiid")) while(hasChildNodes() == true) removeChild(lastChild);
	
	// 分類に一致する金融機関リストを登録する
	for(i in lists) {
		tag_option = dom_create_tag("option", { "value": i });
		tag_option.appendChild(dom_create_text(lists[i]["name"]));
		dom_get_id("fiid").appendChild(tag_option);
	}
	
	// 金融機関リストの先頭を選択する
	dom_get_id("fiid").selectedIndex = 0;
}

// 変更機能
function fnc_modify(rowid) {
	var logons = local_current();
	var body = document.createDocumentFragment();
	var lists = new Array();
	var auths = new Array();
	var auth = fnc_getauth(rowid);
	var fiid;
	var tag_p;
	var inputs, settings;
	var i, j;
	
	if(dom_get_id("modal") == null) {
		if(typeof auth != "undefined") {
			lists = auth.split("\t");
			for(i = 0; i < lists.length; i++) {
				j = lists[i].indexOf("=");
				if(j != -1) auths[lists[i].substring(0, j)] = lists[i].substring(j + 1);
			}
		}
		
		fiid = rowid.split("=")[1];
		// 入力項目を設定する
		inputs = fiids[fiid]["form"].split(",");
		for(i = 0; i < inputs.length; i++) {
			lists = fiids[fiid][inputs[i]].split(",", 2);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(lists[0]));
			body.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "value": (typeof auths[inputs[i]] == "string"? auths[inputs[i]]: "") , "class": "ipt", "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			body.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		if(typeof auth == "string") tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show(fiids[fiid]["name"], body, true, inputs[0]);
		
		// 変更画面の未入力項目をチェックする
		form_empty_check();
	} else {
		// 認証情報を生成する
		fiid = dom_get_id("fiid").value;
		auth = (dom_get_id("auth") != null? dom_get_id("auth").value: null);
		inputs = fiids[fiid]["form"].split(",");
		auths.push("=" + fiid);
		for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
		modal_hide();
		
		if(auth != null) {
			settings = auth_parse(auth);
			if(settings["rownum"] != -1) {
				// OFXを削除する
				dom_del_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
				// 認証情報を更新する
				logoninfo_update(auths.join("\t"), auth);
			} else {
				// 認証情報を追加する
				logoninfo_add(auths.join("\t"));
			}
		} else {
			// 認証情報を追加する
			logoninfo_add(auths.join("\t"));
		}
	}
	return false;
}

// 削除機能
function fnc_delete(rowid) {
	var body = document.createDocumentFragment();
	var auth;
	var fiid;
	var tag_p;
	
	if(dom_get_id("auth") == null) {
		auth = fnc_getauth(rowid);
		fiid = rowid.split("=")[1];
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text(fiids[fiid]["name"] + "を削除します。"));
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
		body.appendChild(tag_p);
		
		// ダイアログを開く
		modal_show("削除", body, true);
		return false;
	} else {
		if(typeof auth != "string") auth = dom_get_id("auth").value;
		
		// コールバックの場合
		modal_hide();
		
		logoninfo_delete(auth);
	}
}

// 更新機能
function fnc_update(rowid, additional) {
	var auth = fnc_getauth(rowid);
	var auths = auth.split("\t");
	var querys = new Array();
	var token = "";
	
	var fiid;
	var query, status;
	var i, j, k, l, m;
	
	for(i = 0; i < auths.length; i++) {
		j = auths[i].indexOf("=");
		if(j != -1) {
			k = auths[i].substring(0, j);
			l = auths[i].substring(j + 1);
			if(i == 0) {
				m = parseInt(k, 10);
				fiid = l;
				k = "fiid";
			}
			
			if(i == 0 || fiids[fiid]["form"].indexOf(k) != -1) querys.push(k + "=" + encodeURIComponent(l));
			if(k == "status") status = l;
			if(k == "token") token = l;
		}
	}
	
	if(status == "202" && typeof additional == "undefined") {
		fnc_update_additional(auth);
		return false;
	}
	
	if(token != "") querys.push("X-Token=" + token);
	if(typeof additional != "undefined") querys.push(additional);
	query = querys.join("&");
	querys[0] = m + "=" + fiid;
	if(typeof additional != "undefined") querys.pop();
	
	xhr = new XMLHttpRequest();
	with(xhr) {
		onreadystatechange = function() {
			var logons, ofx, inputs, token, query;
			var i;
			
			if(xhr != null && xhr.readyState == 4) {
				var logons = local_current();
				
				if(xhr.status != 0 && xhr.status != 204) {
					// OFXを設定する
					ofx = xhr.responseText;
					dom_set_storage(logons["localid"] + ":" + querys[0], xhr.responseText, logons["localpass"]);
				}
				
				// 変更・削除・更新・明細・OFXボタンの押下を許可する
				inputs = dom_get_id("account").getElementsByTagName("input");
				for(i = 0; i < inputs.length; i++) {
					switch(inputs[i].value) {
					case "変更":
					case "削除":
					case "更新":
					case "明細":
					case "OFX":
						inputs[i].disabled = false;
						break;
					default:
						break;
					}
				}
				
				// ログオフボタンの押下を許可する
				dom_get_id("btn_logoff").disabled = false;
				
				// バージョン情報ボタンの押下を許可する
				dom_get_id("btn_version").disabled = false;
				
				// 追加ボタンの押下を許可する
				dom_get_id("btn_add").disabled = false;
				
				// 設定ボタンの押下を許可する
				dom_get_id("btn_option").disabled = false;
				
				// 中止ボタンの押下を禁止する
				dom_get_id("btn_get_stop").disabled = true;
				
				dom_get_id("main").className = "";
				dom_get_id(auths[0]).className = "";
				
				// 口座一覧の項目を更新する
				if(xhr.status != 0) {
					querys.push("status=" + xhr.status.toString());
					querys.push("timestamp=" + timestamp_get());
					
					token = xhr.getResponseHeader("X-Token");
					if(token != null && token != "") {
						i = token.indexOf(",");
						if(i != -1) token = token.substring(0, i);
						querys.push("token=" + token);
					}
					
					query = decodeURIComponent(querys.join("\t"));
					
					logoninfo_update(query, auth);
				}
				
				if(get_all != -1) fnc_update_all(auths[0]);
			}
		};
		open("POST", "./server.php?fiid=" + fiid, true);
		setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
		send(query);
	}
	
	var inputs;
	
	// 変更・削除・更新・明細・OFXボタンの押下を禁止する
	inputs = dom_get_id("account").getElementsByTagName("input");
	for(i = 0; i < inputs.length; i++) {
		switch(inputs[i].value) {
		case "変更":
		case "削除":
		case "更新":
		case "明細":
		case "OFX":
			inputs[i].disabled = true;
			break;
		default:
			break;
		}
	}
	
	// ログオフボタンの押下を禁止する
	dom_get_id("btn_logoff").disabled = true;
	
	// バージョン情報ボタンの押下を禁止する
	dom_get_id("btn_version").disabled = true;
	
	// 追加ボタンの押下を禁止する
	dom_get_id("btn_add").disabled = true;
	
	// 設定ボタンの押下を許可する
	dom_get_id("btn_option").disabled = true;
	
	// 中止ボタンの押下を許可する
	dom_get_id("btn_get_stop").disabled = false;
	
	dom_get_id("main").className = "pending";
	dom_get_id(auths[0]).className = "pending";
	
	return xhr;
}

// すべて更新機能
function fnc_update_all(auth) {
	var logons = local_current();
	var auths = dom_get_storage(logons["localid"], logons["localpass"]).split("\r\n");
	var rowid;
	
	if(typeof auth != "string" || auth.indexOf("=") == -1) {
		rowid = 0;
	} else {
		rowid = parseInt(auth.substring(0, auth.indexOf("=")), 10) + 1;
	}
	
	if(typeof auths[rowid] != "undefined") {
		var settings = auth_parse(auths[rowid]);
		get_all = rowid;
		fnc_update(settings["rowid"]);
	} else {
		get_all = -1;
	}
}

// 追加認証画面を表示する
function fnc_update_additional(auth) {
	var body = document.createDocumentFragment();
	var auths = new Array();
	var querys = new Array();
	var group = "";
	var status = "";
	var timestamp = "";
	var ofx = null;
	var parser = null;
	var tag_p;
	var logons, settings, str, inputs, query;
	var sesscookie, accesskey, mfaphraseid, mfaphraselabel;
	var i, j, k, l;
	
	if(dom_get_id("modal") == null) {
		logons = local_current();
		settings = auth_parse(auth);
		str = dom_get_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
		auths = auth.split("\t");
		
		if(chkenv_parser() == true) parser = new DOMParser();
		
		switch(str) {
		case null:
		case "":
			break;
		default:
			if(parser != null) ofx = parser.parseFromString(str, "text/xml");
			break;
		}
		
		sesscookie = (ofx != null? ofx.getElementsByTagName("SESSCOOKIE")[0].firstChild.nodeValue: "");
		accesskey = (ofx != null? ofx.getElementsByTagName("ACCESSKEY")[0].firstChild.nodeValue: "");
		mfaphraseid = (ofx != null? ofx.getElementsByTagName("MFAPHRASEID")[0].firstChild.nodeValue: "");
		mfaphraselabel = (ofx != null? ofx.getElementsByTagName("MFAPHRASELABEL")[0].firstChild.nodeValue: "");
		
		// 追加認証情報を生成する
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "additional", "id": "additional", "value": mfaphraseid }));
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "sesscookie", "id": "sesscookie", "value": sesscookie }));
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "accesskey", "id": "accesskey", "value": accesskey }));
		body.appendChild(tag_p);
		
		inputs = fiids[settings["fiid"]]["auth"].split(",");
		
		tag_p = dom_create_tag("p");
		if(inputs[2] == "image") {
			tag_p.appendChild(dom_create_tag("img", { "src": mfaphraselabel, "alt": "画像" }));
		} else {
			tag_p.appendChild(dom_create_text(mfaphraselabel));
		}
		body.appendChild(tag_p);
		
		if(inputs[1] != "hidden") {
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(inputs[0]));
			body.appendChild(tag_p);
			
			// 入力項目を設定する
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": inputs[1], "name": mfaphraseid, "id": mfaphraseid, "class": "ipt", "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			body.appendChild(tag_p);
		}
		
		// ダイアログを開く
		modal_show("追加認証", body, true, (inputs[1] != "hidden"? mfaphraseid: "modalok"));
		
		// 追加認証画面の未入力項目をチェックする
		form_empty_check();
	} else {
		// コールバックの場合
		auth = dom_get_id("auth").value;
		
		// 追加認証情報を生成する
		querys.push("sesscookie=" + dom_get_id("sesscookie").value);
		querys.push("accesskey=" + dom_get_id("accesskey").value);
		if(dom_get_id(dom_get_id("additional").value) != null) querys.push(dom_get_id("additional").value + "=" + encodeURIComponent(dom_get_id(dom_get_id("additional").value).value));
		query = querys.join("&");
		
		modal_hide();
		settings = auth_parse(auth);
		fnc_update(settings["rowid"], query);
	}
	return false;
}

// 中止機能
function fnc_cancel() {
	get_all = -1;
	xhr.abort();
	
	// 中止ボタンの押下を禁止する
	dom_get_id("btn_get_stop").disabled = true;
}

// 明細機能
function fnc_detail(rowid) {
	var body = document.createDocumentFragment();
	var tag_p, tag_select, tag_option, tag_div, tag_table, tag_thead, tag_tbody, tag_tr, tag_th, tag_td;
	var tag_seclists, tag_stmttrnrss, tag_stmttrns, tag_invtrans, tag_secids, tag_totals, tag_invposlists;
	var parser = null;
	var securitys = new Array();
	var auth, logons, settings, str, current;
	var uniqueidtypes, uniqueids, secnames, mktginfo, dtposted, name, trnamt, dttrade, security, total, dtpriceasofs, mktvals;
	var i, j;
	
	if(chkenv_parser() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、明細の表示に対応していません。", false);
	} else {
		parser = new DOMParser();
		
		if(dom_get_id("modal") == null) {
			auth = fnc_getauth(rowid);
			logons = local_current();
			settings = auth_parse(auth);
			
			str = dom_get_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			if(str != null && str != "") {
				current = parser.parseFromString(str, "text/xml");
				
				// 証券一覧を生成する
				tag_seclists = current.getElementsByTagName("SECLIST");
				if(tag_seclists.length > 0) {
					uniqueidtypes = tag_seclists[0].getElementsByTagName("UNIQUEIDTYPE");
					uniqueids = tag_seclists[0].getElementsByTagName("UNIQUEID");
					secnames = tag_seclists[0].getElementsByTagName("SECNAME");
					for(i = 0; i < secnames.length; i++) securitys[uniqueidtypes[i].firstChild.nodeValue + " " + uniqueids[i].firstChild.nodeValue] = secnames[i].firstChild.nodeValue;
				}
				
				// 口座種目を生成する
				tag_p = dom_create_tag("p", { "class": "label" });
				tag_p.appendChild(dom_create_text("口座種目"));
				body.appendChild(tag_p);
				
				tag_stmttrnrss = current.getElementsByTagName("STMTTRNRS");
				if(tag_stmttrnrss.length == 0) tag_stmttrnrss = current.getElementsByTagName("CCSTMTTRNRS");
				if(tag_stmttrnrss.length == 0) tag_stmttrnrss = current.getElementsByTagName("INVSTMTTRNRS");
				
				if(tag_stmttrnrss.length == 0) {
					modal_showonly("警告", "表示可能な明細がありません。", false);
				} else {
					tag_select = dom_create_tag("select", { "id": "acct", "onchange": "fnc_detail_change();" });
					for(i = 0; i < tag_stmttrnrss.length; i++) {
						mktginfo = (tag_stmttrnrss[i].getElementsByTagName("MKTGINFO").length == 0? "": tag_stmttrnrss[i].getElementsByTagName("MKTGINFO")[0].firstChild.nodeValue);
						j = mktginfo.indexOf("　");
						group = (j == -1? "預金": mktginfo.substring(j + 1)) + " " + tag_stmttrnrss[i].getElementsByTagName("ACCTID")[0].firstChild.nodeValue;
						
						tag_option = dom_create_tag("option", { "value": i.toString() });
						tag_option.appendChild(dom_create_text(str_to_hankaku(group)));
						tag_select.appendChild(tag_option);
					}
					
					body.appendChild(tag_select);
					
					// 一覧を生成する
					tag_div = dom_create_tag("div", { "id": "details" });
					tag_table = dom_create_tag("table", { "id": "detail" });
					tag_thead = dom_create_tag("thead");
					tag_tr = dom_create_tag("tr");
					
					tag_th = dom_create_tag("th", { "class": "dt" });
					tag_th.appendChild(dom_create_text("日付"));
					tag_tr.appendChild(tag_th);
					
					tag_th = dom_create_tag("th", { "class": "note" });
					tag_th.appendChild(dom_create_text("摘要"));
					tag_tr.appendChild(tag_th);
					
					tag_th = dom_create_tag("th", { "class": "amt" });
					tag_th.appendChild(dom_create_text("金額"));
					tag_tr.appendChild(tag_th);
					
					tag_thead.appendChild(tag_tr);
					tag_table.appendChild(tag_thead);
					
					for(i = 0; i < tag_stmttrnrss.length; i++) {
						tag_stmttrns = tag_stmttrnrss[i].getElementsByTagName("STMTTRN");
						tag_tbody = dom_create_tag("tbody", { "id": "acct" + i.toString() });
						
						if(tag_stmttrns.length > 0) {
							for(j = 0; j < tag_stmttrns.length; j++) {
								dtposted = tag_stmttrns[j].getElementsByTagName("DTPOSTED")[0].firstChild.nodeValue;
								name = tag_stmttrns[j].getElementsByTagName("NAME")[0].firstChild.nodeValue;
								trnamt = tag_stmttrns[j].getElementsByTagName("TRNAMT")[0].firstChild.nodeValue;
								
								tag_tr = dom_create_tag("tr");
								
								tag_td = dom_create_tag("td", { "class": "ac" });
								tag_td.appendChild(dom_create_text(parseInt(dtposted.substring(4, 6), 10).toString() + "/" + parseInt(dtposted.substring(6, 8), 10).toString()));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td");
								tag_td.appendChild(dom_create_text(str_to_hankaku(name)));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "ar" });
								tag_td.appendChild(dom_create_text(to_amount(trnamt)));
								tag_tr.appendChild(tag_td);
								
								tag_tbody.appendChild(tag_tr);
							}
						} else {
							tag_tr = dom_create_tag("tr");
							
							tag_td = dom_create_tag("td", { "colspan": "3", "class": "ac" } );
							tag_td.appendChild(dom_create_text("明細がありません。"));
							tag_tr.appendChild(tag_td);
							
							tag_tbody.appendChild(tag_tr);
						}
						
						tag_table.appendChild(tag_tbody);
						
						
						// 売買一覧を生成する
						tag_invtrans = tag_stmttrnrss[i].getElementsByTagName("INVTRAN");
						tag_secids = tag_stmttrnrss[i].getElementsByTagName("SECID");
						tag_totals = tag_stmttrnrss[i].getElementsByTagName("TOTAL");
						
						if(tag_invtrans.length > 0) {
							tag_tbody = dom_create_tag("tbody", { "id": "acct" + tag_stmttrnrss.length.toString() });
							
							tag_option = dom_create_tag("option", { "value": tag_stmttrnrss.length.toString() });
							tag_option.appendChild(dom_create_text("売買"));
							tag_select.appendChild(tag_option);
							
							for(j = 0; j < tag_invtrans.length; j++) {
								dttrade = tag_invtrans[j].getElementsByTagName("DTTRADE")[0].firstChild.nodeValue;
								security = securitys[tag_secids[j].getElementsByTagName("UNIQUEIDTYPE")[0].firstChild.nodeValue + " " + tag_secids[j].getElementsByTagName("UNIQUEID")[0].firstChild.nodeValue];
								total = tag_totals[j].firstChild.nodeValue;
								
								tag_tr = dom_create_tag("tr");
								
								tag_td = dom_create_tag("td", { "class": "ac" });
								tag_td.appendChild(dom_create_text(parseInt(dttrade.substring(4, 6), 10).toString() + "/" + parseInt(dttrade.substring(6, 8), 10).toString()));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td");
								tag_td.appendChild(dom_create_text(str_to_hankaku((parseInt(total, 10) < 0? "買付": "売付") + " " + security)));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "ar" });
								tag_td.appendChild(dom_create_text(to_amount(Math.abs(total))));
								tag_tr.appendChild(tag_td);
								
								tag_tbody.appendChild(tag_tr);
							}
							
							tag_tbody.appendChild(tag_tr);
							tag_table.appendChild(tag_tbody);
						}
						
						// 有価証券残高一覧を生成する
						tag_invposlists = tag_stmttrnrss[i].getElementsByTagName("INVPOSLIST");
						
						if(tag_invposlists.length > 0) {
							uniqueidtypes = tag_invposlists[0].getElementsByTagName("UNIQUEIDTYPE");
							uniqueids = tag_invposlists[0].getElementsByTagName("UNIQUEID");
							dtpriceasofs = tag_invposlists[0].getElementsByTagName("DTPRICEASOF");
							mktvals = tag_invposlists[0].getElementsByTagName("MKTVAL");
							
							if(dtpriceasofs.length > 0) {
								tag_tbody = dom_create_tag("tbody", { "id": "acct" + (tag_stmttrnrss.length + 1).toString() });
								
								tag_option = dom_create_tag("option", { "value": (tag_stmttrnrss.length + 1).toString() });
								tag_option.appendChild(dom_create_text("有価証券残高"));
								tag_select.appendChild(tag_option);
								
								for(j = 0; j < dtpriceasofs.length; j++) {
									dtpriceasof = dtpriceasofs[j].firstChild.nodeValue;
									name = securitys[uniqueidtypes[j].firstChild.nodeValue + " " + uniqueids[j].firstChild.nodeValue];
									mktval = mktvals[j].firstChild.nodeValue;
									
									tag_tr = dom_create_tag("tr");
									
									tag_td = dom_create_tag("td", { "class": "ac" });
									tag_td.appendChild(dom_create_text(parseInt(dtpriceasof.substring(4, 6), 10).toString() + "/" + parseInt(dtpriceasof.substring(6, 8), 10).toString()));
									tag_tr.appendChild(tag_td);
									
									tag_td = dom_create_tag("td");
									tag_td.appendChild(dom_create_text(str_to_hankaku(name)));
									tag_tr.appendChild(tag_td);
									
									tag_td = dom_create_tag("td", { "class": "ar" });
									tag_td.appendChild(dom_create_text(to_amount(mktval)));
									tag_tr.appendChild(tag_td);
									
									tag_tbody.appendChild(tag_tr);
								}
								
								tag_tbody.appendChild(tag_tr);
								tag_table.appendChild(tag_tbody);
							}
						}
						
						tag_div.appendChild(tag_table);
						body.appendChild(tag_div);
					}
					
					// ダイアログを開く
					modal_show("明細", body, false, "acct");
					
					fnc_detail_change();
				}
			}
		} else {
			// ダイアログを閉じる
			modal_hide();
		}
	}
	return false;
}

// 選択中以外の明細を非表示にする
function fnc_detail_change() {
	var acct = dom_get_id("acct").value;
	var obj;
	var i;
	
	for(i = 0; i < 100; i++) {
		obj = dom_get_id("acct" + i.toString());
		if(obj != null) obj.style.display = (i == acct)? "table-row-group": "none";
	}
}

// OFXダウンロード機能
function fnc_ofx(rowid) {
	var auth = fnc_getauth(rowid);
	var logons = local_current();
	var settings = auth_parse(auth);
	var ofx = null;
	var tag_a;
	var filename, url, db;
	
	if(chkenv_ofx() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、OFXのダウンロードに対応していません。", false);
	} else {
		ofx = new Blob([dom_get_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"])]);
		filename = settings["fiid"] + (settings["keyvalues"]["timestamp"] != ""? "_" + settings["keyvalues"]["timestamp"]: "") + ".ofx";
		
		if(self.window.navigator.msSaveOrOpenBlob) {
			self.window.navigator.msSaveOrOpenBlob(ofx, filename);
		} else {
			url = self.window.URL || self.window.webkitURL;
			db = dom_get_tag("body")[0];
			tag_a = dom_create_tag("a", { "href": url.createObjectURL(ofx), "id": "download", "type": "application/x-ofx; charset=UTF-8", "download": filename });
			tag_a.appendChild(dom_create_text("ダウンロード"));
			db.appendChild(tag_a);
			dom_get_id("download").click();
			db.removeChild(tag_a);
		}
		ofx = null;
	}
}

// OFX結合ダウンロード機能
function fnc_ofx_all() {
	var parser = null;
	var serializer = null;
	var merge = null;
	var current = null;
	var f = false;
	var logons, auths, timestamp, settings, filename, str, ofx;
	var tag_ofx, tag_signonmsgsrsv1, tag_sonrs, tag_status, tag_code, tag_severity, tag_dtserver, tag_language, tag_fi, tag_org, tag_bankmsgsrsv1, tag_creditcardmsgsrsv1, tag_invstmtmsgsrsv1, tag_seclistmsgsrsv1, tag_stmttrnrss, tag_seclist, tag_ccstmttrnrss, tag_invstmttrnrss, tag_seclists, tag_seclisttrnrs, tag_trnuid;
	var tag_a;
	var url, db;
	var i, j, k;
	
	if(chkenv_ofx_all() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、OFXの結合ダウンロードに対応していません。", false);
	} else {
		parser = new DOMParser();
		serializer = new XMLSerializer();
		merge = parser.parseFromString("<OFX></OFX>", "text/xml");
		logons = local_current();
		auths = dom_get_storage(logons["localid"], logons["localpass"]).split("\r\n");
		timestamp = timestamp_get();
		
		tag_ofx = merge.firstChild;
		
		tag_signonmsgsrsv1 = merge.createElement("SIGNONMSGSRSV1");
		tag_sonrs = merge.createElement("SONRS");
		
		tag_status = merge.createElement("STATUS");
		
		tag_code = merge.createElement("CODE");
		tag_code.appendChild(merge.createTextNode("0"));
		tag_status.appendChild(tag_code);
		
		tag_severity = merge.createElement("SEVERITY");
		tag_severity.appendChild(merge.createTextNode("INFO"));
		tag_status.appendChild(tag_severity);
		
		tag_sonrs.appendChild(tag_status);
		
		tag_dtserver = merge.createElement("DTSERVER");
		tag_dtserver.appendChild(merge.createTextNode(timestamp + "[+9:JST]"));
		tag_sonrs.appendChild(tag_dtserver);
		
		tag_language = merge.createElement("LANGUAGE");
		tag_language.appendChild(merge.createTextNode("JPN"));
		tag_sonrs.appendChild(tag_language);
		
		tag_fi = merge.createElement("FI");
		
		tag_org = merge.createElement("ORG");
		tag_org.appendChild(merge.createTextNode("MoneySound/" + rel));
		tag_fi.appendChild(tag_org);
		tag_sonrs.appendChild(tag_fi);
		
		tag_signonmsgsrsv1.appendChild(tag_sonrs);
		tag_ofx.appendChild(tag_signonmsgsrsv1);
		
		tag_bankmsgsrsv1 = merge.createElement("BANKMSGSRSV1");
		tag_creditcardmsgsrsv1 = merge.createElement("CREDITCARDMSGSRSV1");
		tag_invstmtmsgsrsv1 = merge.createElement("INVSTMTMSGSRSV1");
		tag_seclistmsgsrsv1 = merge.createElement("SECLISTMSGSRSV1");
		
		tag_seclisttrnrs = merge.createElement("SECLISTTRNRS");
		tag_seclist = merge.createElement("SECLIST");
		
		for(i = 0; i < auths.length; i++) {
			settings = auth_parse(auths[i]);
			
			str = dom_get_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			if(str != null && str != "") {
				try {
					current = parser.parseFromString(str, "text/xml");
				} catch(e) {
					// 何もしない
					void(0);
				}
				
				// 銀行
				tag_stmttrnrss = (current != null? current.getElementsByTagName("STMTTRNRS"): new Array());
				for(j = 0; j < tag_stmttrnrss.length; j++) {
					tag_bankmsgsrsv1.appendChild(tag_stmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// クレジットカード
				tag_ccstmttrnrss = (current != null? current.getElementsByTagName("CCSTMTTRNRS"): new Array());
				for(j = 0; j < tag_ccstmttrnrss.length; j++) {
					tag_creditcardmsgsrsv1.appendChild(tag_ccstmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// 証券
				tag_invstmttrnrss = (current != null? current.getElementsByTagName("INVSTMTTRNRS"): new Array());
				for(j = 0; j < tag_invstmttrnrss.length; j++) {
					tag_invstmtmsgsrsv1.appendChild(tag_invstmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// 証券
				tag_seclists = (current != null? current.getElementsByTagName("SECLIST"): new Array());
				for(j = 0; j < tag_seclists.length; j++) for(k = 0; k < tag_seclists[j].childNodes.length; k++) if(tag_seclists[j].childNodes[k].nodeType == 1) tag_seclist.appendChild(tag_seclists[j].childNodes[k].cloneNode(true));
			}
		}
		
		// STMTTRNRSが空でない場合、BANKMSGSRSV1を追加する
		if(tag_bankmsgsrsv1.childNodes.length > 0) tag_ofx.appendChild(tag_bankmsgsrsv1);
		
		// CCSTMTTRNRSが空でない場合、CREDITCARDMSGSRSV1を追加する
		if(tag_creditcardmsgsrsv1.childNodes.length > 0) tag_ofx.appendChild(tag_creditcardmsgsrsv1);
		
		// INVSTMTTRNRSが空でない場合、INVSTMTMSGSRSV1を追加する
		if(tag_invstmtmsgsrsv1.childNodes.length > 0) tag_ofx.appendChild(tag_invstmtmsgsrsv1);
		
		// SECLISTが空でない場合、SECLISTTRNRSを生成し、SECLISTMSGSRSV1を追加する
		if(tag_seclist.childNodes.length > 0) {
			tag_trnuid = merge.createElement("TRNUID");
			tag_trnuid.appendChild(merge.createTextNode("0"));
			tag_seclisttrnrs.appendChild(tag_trnuid);
			
			tag_status = merge.createElement("STATUS");
			
			tag_code = merge.createElement("CODE");
			tag_code.appendChild(merge.createTextNode("0"));
			tag_status.appendChild(tag_code);
			
			tag_severity = merge.createElement("SEVERITY");
			tag_severity.appendChild(merge.createTextNode("INFO"));
			tag_status.appendChild(tag_severity);
			
			tag_seclisttrnrs.appendChild(tag_status);
			
			tag_seclistmsgsrsv1.appendChild(tag_seclisttrnrs);
			tag_seclistmsgsrsv1.appendChild(tag_seclist);
			
			tag_ofx.appendChild(tag_seclistmsgsrsv1);
		}
		
		str = ofxhead + serializer.serializeToString(merge);
		
		ofx = new Blob([str]);
		filename = "MoneySound_" + timestamp + ".ofx";
		
		if(f == false) {
			modal_showonly("警告", "ダウンロード可能なOFXがありません。", false);
		} else {
			if(self.window.navigator.msSaveOrOpenBlob) {
				self.window.navigator.msSaveOrOpenBlob(ofx, filename);
			} else {
				url = self.window.URL || self.window.webkitURL;
				db = dom_get_tag("body")[0];
				tag_a = dom_create_tag("a", { "href": url.createObjectURL(ofx), "id": "download", "type": "application/x-ofx; charset=UTF-8", "download": filename });
				tag_a.appendChild(dom_create_text("ダウンロード"));
				db.appendChild(tag_a);
				dom_get_id("download").click();
				db.removeChild(tag_a);
			}
		}
	}
}

// 口座一覧表示機能
function fnc_listall(lists) {
	var logons = local_current();
	var tag_table = dom_get_id("account");
	var tag_tbodys = tag_table.getElementsByTagName("tbody");
	var f = false;
	var i;
	
	for(i = tag_tbodys.length - 1; i >= 0; i--) tag_table.removeChild(tag_tbodys[i]);
	
	for(i = 0; i < lists.length; i++) {
		if(lists[i].length == 0) continue;
		tag_table.appendChild(fnc_listone(lists[i]));
		f = true;
	}
	
	if(f == true) {
		dom_get_id("btn_get_all").disabled = false;
		dom_get_id("btn_ofx_all").disabled = false;
	}
	
	// 口座一覧を更新する
	account_total_update();
}

// 口座表示機能
function fnc_listone(list) {
	var logons = local_current();
	var settings = auth_parse(list);
	var str = dom_get_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
	var ofx = null;
	var parser = null;
	var broken = false;
	var group = "";
	var caption = "";
	var status = "";
	var timestamp = "";
	var banks, creditcards, investments;
	var tag_tbody, tag_tr, tag_td, tag_a;
	var balamt, mktginfo, bankacctfrom, bankid, branchid, acctid, accttype, ccacctfrom, stmttrns, stmttrn, marginbalance, invacctfrom, brokerid, acctid, mktval, invposlist, mktvals;
	var i, j;
	
	if(chkenv_parser() == true) parser = new DOMParser();
	
	switch(str) {
	case null:
	case "":
		break;
	default:
		if(parser != null) try {
			ofx = parser.parseFromString(str, "text/xml");
		} catch(e) {
			broken = true;
		}
		break;
	}
	
	tag_tbody = dom_create_tag("tbody", { "id": settings["rowid"] });
	
	for(i in settings["keyvalues"]) {
		switch(i) {
		case "status":
			status = settings["keyvalues"][i];
			break;
		case "timestamp":
			with(settings["keyvalues"][i]) timestamp = parseInt(substring(4, 6), 10).toString() + "/" + parseInt(substring(6, 8), 10).toString() + " " + substring(8, 10) + ":" + substring(10, 12);
			break;
		default:
			break;
		}
	}
	
	switch(status) {
	case "":
		group = "未更新: 更新ボタンを押下してください";
		tag_tbody.className = "ready";
		break;
	case "200":
		// 何もしない
		void(0);
		break;
	case "202":
		group = "追加認証あり: 更新ボタンを押下してください";
		tag_tbody.className = "error";
		break;
	case "400":
		group = "認証失敗: 認証情報が正しいか否かを確認してください";
		caption = "金融機関のサイトにログインできませんでした。認証情報が正しいか否か、および追加認証の入力内容が正しいか否かを確認した後、再試行してください。";
		tag_tbody.className = "error";
		break;
	case "500":
		group = "OFX破損: サーバーの状態を確認してください";
		caption = "サーバーより取得したOFXが破損していました。再試行しても同じエラーが発生する場合、サーバーの管理者にお問い合わせください。";
		tag_tbody.className = "error";
		break;
	case "503":
		group = "メンテナンス中: 金融機関のサイトで日程を確認してください";
		caption = "金融機関のサイトがメンテナンス中でした。メンテナンスが終了した後、再試行してください。";
		tag_tbody.className = "error";
		break;
	case "504":
		group = "重要通知あり: 金融機関のサイトで画面を確認してください";
		caption = "金融機関のサイトに「重要なお知らせ」等の画面が表示されました。金融機関のサイトに直接ログインし、内容を確認した後、再試行してください。";
		tag_tbody.className = "error";
		break;
	default:
		group = "エラー（" + status + "）: サーバーの状態を確認してください";
		caption = "サーバーでエラーが発生しました。再試行しても同じエラーが発生する場合、サーバーの管理者にお問い合わせください。";
		tag_tbody.className = "error";
		break;
	}
	try {
		// 銀行
		banks = (ofx != null? ofx.getElementsByTagName("STMTTRNRS"): new Array());
		for(i = 0; i < banks.length; i++) {
			with(banks[i]) {
				balamt = parseInt(getElementsByTagName("BALAMT")[0].firstChild.nodeValue, 10);
				mktginfo = (getElementsByTagName("MKTGINFO").length == 0? "": getElementsByTagName("MKTGINFO")[0].firstChild.nodeValue);
				bankacctfrom = getElementsByTagName("BANKACCTFROM")[0];
			}
			with(bankacctfrom) {
				bankid = getElementsByTagName("BANKID")[0].firstChild.nodeValue;
				branchid = getElementsByTagName("BRANCHID")[0].firstChild.nodeValue;
				acctid = getElementsByTagName("ACCTID")[0].firstChild.nodeValue;
				accttype = getElementsByTagName("ACCTTYPE")[0].firstChild.nodeValue;
			}
			
			j = mktginfo.indexOf("　");
			group = (j == -1? "預金": mktginfo.substring(j + 1));
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": banks.length.toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": fiids[settings["fiid"]]["home"], "target": settings["fiid"] });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": bankid + " " + branchid + " " + acctid });
			tag_td.appendChild(dom_create_text(str_to_hankaku(group)));
			tag_tr.appendChild(tag_td);
			
			// 残高
			tag_td = dom_create_tag("td", { "class": "balance" });
			tag_td.appendChild(dom_create_text(to_amount(balamt)));
			tag_tr.appendChild(tag_td);
			
			// 状態
			if(i == 0) {
				// 最終更新日時
				tag_td = dom_create_tag("td", { "rowspan": banks.length.toString(), "class": "timestamp" });
				tag_td.appendChild(dom_create_text(timestamp));
				tag_tr.appendChild(tag_td);
				
				// 操作
				tag_td = dom_create_tag("td", { "rowspan": banks.length.toString(), "class": "control" });
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "class": "btn", "onclick": "fnc_update(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "class": "btn", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "class": "btn", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "class": "btn", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "class": "btn", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
		}
		
		// クレジットカード
		creditcards = (ofx != null? ofx.getElementsByTagName("CCSTMTTRNRS"): new Array());
		for(i = 0; i < creditcards.length; i++) {
			with(creditcards[i]) {
				balamt = parseInt(getElementsByTagName("BALAMT")[0].firstChild.nodeValue, 10);
				mktginfo = (getElementsByTagName("MKTGINFO").length == 0? "": getElementsByTagName("MKTGINFO")[0].firstChild.nodeValue);
				ccacctfrom = getElementsByTagName("CCACCTFROM")[0];
			}
			acctid = ccacctfrom.getElementsByTagName("ACCTID")[0].firstChild.nodeValue;
			
			j = mktginfo.indexOf("　");
			group = (j == -1? "預金": mktginfo.substring(j + 1));
			
			// 明細の最終行がクレジットカード支払請求の場合、残高より該当金額を差し引く
			stmttrns = creditcards[i].getElementsByTagName("STMTTRN");
			stmttrn = stmttrns[stmttrns.length - 1];
			if(stmttrn.getElementsByTagName("NAME")[0].firstChild.nodeValue == fiids[settings["fiid"]]["name"]) balamt -= parseInt(stmttrn.getElementsByTagName("TRNAMT")[0].firstChild.nodeValue, 10);
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": creditcards.length.toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": fiids[settings["fiid"]]["home"], "target": settings["fiid"] });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": acctid });
			tag_td.appendChild(dom_create_text(str_to_hankaku(group)));
			tag_tr.appendChild(tag_td);
			
			// 残高
			tag_td = dom_create_tag("td", { "class": "balance" });
			tag_td.appendChild(dom_create_text(to_amount(balamt)));
			tag_tr.appendChild(tag_td);
			
			if(i == 0) {
				// 最終更新日時
				tag_td = dom_create_tag("td", { "class": "timestamp" });
				tag_td.appendChild(dom_create_text(timestamp));
				tag_tr.appendChild(tag_td);
				
				// 操作
				tag_td = dom_create_tag("td", { "rowspan": banks.length.toString(), "class": "control" });
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "class": "btn", "onclick": "fnc_update(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "class": "btn", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "class": "btn", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "class": "btn", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "class": "btn", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
		}
		
		// 証券
		investments = (ofx != null? ofx.getElementsByTagName("INVSTMTTRNRS"): new Array());
		for(i = 0; i < investments.length; i++) {
			with(investments[i]) {
				marginbalance = parseInt(investments[i].getElementsByTagName("MARGINBALANCE")[0].firstChild.nodeValue, 10);
				// mktginfo = (getElementsByTagName("MKTGINFO").length == 0? "": getElementsByTagName("MKTGINFO")[0].firstChild.nodeValue);
				invacctfrom = getElementsByTagName("INVACCTFROM")[0];
			}
			with(invacctfrom) {
				brokerid = getElementsByTagName("BROKERID")[0].firstChild.nodeValue;
				acctid = getElementsByTagName("ACCTID")[0].firstChild.nodeValue;
			}
			
			group = "預金";
			
			mktval = 0;
			invposlist = investments[i].getElementsByTagName("INVPOSLIST")[0];
			mktvals = invposlist.getElementsByTagName("MKTVAL");
			for(j = 0; j < mktvals.length; j++) mktval += parseInt(mktvals[j].firstChild.nodeValue, 10);
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": (investments.length + 1).toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": fiids[settings["fiid"]]["home"], "target": settings["fiid"] });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": brokerid + " " + acctid });
			tag_td.appendChild(dom_create_text(str_to_hankaku(group)));
			tag_tr.appendChild(tag_td);
			
			// 残高（預金）
			tag_td = dom_create_tag("td", { "class": "balance" });
			tag_td.appendChild(dom_create_text(to_amount(marginbalance)));
			tag_tr.appendChild(tag_td);
			
			if(i == 0) {
				// 最終更新日時
				tag_td = dom_create_tag("td", { "rowspan": (investments.length + 1).toString(), "class": "timestamp" });
				tag_td.appendChild(dom_create_text(timestamp));
				tag_tr.appendChild(tag_td);
				
				// 操作
				tag_td = dom_create_tag("td", { "rowspan": (investments.length + 1).toString(), "class": "control" });
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "class": "btn", "onclick": "fnc_update(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "class": "btn", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "class": "btn", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "class": "btn", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "class": "btn", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
			
			tag_tr = dom_create_tag("tr");
			
			group = "有価証券";
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": brokerid + " " + acctid + "-1" });
			tag_td.appendChild(dom_create_text(str_to_hankaku(group)));
			tag_tr.appendChild(tag_td);
			
			// 残高（有価証券）
			tag_td = dom_create_tag("td", { "class": "balance" });
			tag_td.appendChild(dom_create_text(to_amount(mktval)));
			tag_tr.appendChild(tag_td);
			
			tag_tbody.appendChild(tag_tr);
		}
	} catch(e) {
		broken = true;
	}
	
	if(broken == true) {
		status = "500";
		group = "OFX破損: サーバーの状態を確認してください";
		caption = "サーバーより取得したOFXが破損していました。再試行しても同じエラーが発生する場合、サーバーの管理者にお問い合わせください。";
		tag_tbody.className = "error";
	}
	
	// statusが200、かつデータなしの場合
	if(status == 200 && banks.length == 0 && creditcards.length == 0 && investments.length == 0) {
		status = "204";
		group = "データなし: 金融機関のサイトで状況を確認してください";
		caption = "サーバーより取得したデータが空でした。金融機関のサイトに直接ログインし、状況を確認した後、再試行してください。";
		tag_tbody.className = "error";
		broken = true;
	}
	
	// OFX破損、またはデータなしの場合
	if(broken == true || (banks.length == 0 && creditcards.length == 0 && investments.length == 0)) {
		tag_tr = dom_create_tag("tr");
		
		// 金融機関名称
		tag_td = dom_create_tag("td", { "rowspan": "1", "class": "fi" });
		tag_a = dom_create_tag("a", { "href": fiids[settings["fiid"]]["home"], "target": settings["fiid"] });
		tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
		tag_td.appendChild(tag_a);
		tag_tr.appendChild(tag_td);
		
		// 口座種目
		tag_td = dom_create_tag("td", { "colspan": (status == "200"? "1": (timestamp != ""? "2": "3")), "class": "accttype", "title": caption });
		tag_td.appendChild(dom_create_text(str_to_hankaku(group)));
		tag_tr.appendChild(tag_td);
		
		// 残高
		if(status == "200") {
			tag_td = dom_create_tag("td", { "class": "balance" });
			tag_td.appendChild(dom_create_text(""));
			tag_tr.appendChild(tag_td);
		}
		
		// 最終更新日時
		if(timestamp != "") {
			tag_td = dom_create_tag("td", { "class": "timestamp" });
			tag_td.appendChild(dom_create_text(timestamp));
			tag_tr.appendChild(tag_td);
		}
		
		// 操作
		tag_td = dom_create_tag("td", { "class": "control" });
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "class": "btn", "onclick": "fnc_update(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		if(status != "200" && parser != null) {
			tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "class": "btn", "disabled": "true", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		} else {
			tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "class": "btn", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		}
		if(status != "200" && parser != null && debug == false) {
			tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "class": "btn", "disabled": "true", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		} else {
			tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "class": "btn", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		}
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "class": "btn", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "class": "btn", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");", "onkeypress": "this.onclick();" }));
		tag_tr.appendChild(tag_td);
		
		tag_tbody.appendChild(tag_tr);
	}
	
	return tag_tbody;
}


// =========================================================================
// 処理
// =========================================================================

// ダイアログを開く
function modal_show(head, body, showcancel, focusto) {
	var db = dom_get_tag("body")[0];
	var dragging = false;
	var tag_div, tag_h2, tag_form;
	var x, y;
	
	if(dom_get_id("modal") == null) {
		// モーダルウィンドウを生成する
		tag_form = dom_create_tag("form", { "method": "post", "id": "modal", "onsubmit": "(" + arguments.callee.caller + ")(); return false;", "onreset": "modal_hide();" });
		
		tag_h2 = dom_create_tag("h2", { "id": "modalhead" });
		tag_h2.appendChild(dom_create_text(head));
		tag_form.appendChild(tag_h2);
		
		tag_div = dom_create_tag("div", { "id": "modalbody" });
		tag_div.appendChild((typeof body == "string"? dom_create_text(body): body));
		tag_form.appendChild(tag_div);
		
		tag_div = dom_create_tag("div", { "id": "modalfoot" });
		tag_div.appendChild(dom_create_tag("input", { "type": "submit", "value": "OK", "id": "modalok", "class": "btn" }));
		if(showcancel == true) tag_div.appendChild(dom_create_tag("input", { "type": "reset", "value": "キャンセル", "id": "modalcancel", "class": "btn" }));
		
		tag_form.appendChild(tag_div);
		db.appendChild(tag_form);
		
		tag_h2.onmousedown = function(e) {
			if(typeof e == "undefined") e = self.window.event;
			var target = e.target || e.srcElement;
			dragging = true;
			with(this.parentNode) {
				x = e.clientX - offsetLeft;
				y = e.clientY - offsetTop;
			}
			return false;
		};
		with(self.document) {
			onkeydown = function(e) {
				var ret = true;
				if(typeof e == "undefined") e = event;
				if(e != null && tag_form != null) switch(e.keyCode) {
				case 13: // enter
					if(dom_get_id("modalok").disabled == false) tag_form.onsubmit();
					ret = false;
					break;
				case 27: // esc
					tag_form.onreset();
					ret = false;
					break;
				}
				return ret;
			};
			onmouseup = function() {
				dragging = false;
				return false;
			};
			onmousemove = function(e) {
				if(typeof e == "undefined") e = self.window.event;
				if(dragging == true) with(tag_h2.parentNode.style) {
					left = e.clientX - x + "px";
					top = e.clientY - y + "px";
					return false;
				}
			};
		}
		
		// ウィンドウサイズが変更された場合、オーバーレイを再表示する
		self.window.onresize = function() {
			modal_resize();
			return false;
		};
		
		with(dom_get_id(typeof focusto != "string"? "modalok": focusto)) {
			focus();
			if(tagName == "input") select();
		}
		
		// オーバーレイを生成する
		tag_div = dom_create_tag("div", { "id": "overlay" });
		db.appendChild(tag_div);
		
		modal_resize();
	}
}

function modal_showonly(head, body, showcancel, focusto) {
	if(dom_get_id("modal") == null) {
		// ダイアログを開く
		modal_show(head, body, showcancel, focusto);
	} else {
		// コールバックの場合
		modal_hide();
	}
}

function modal_resize() {
	var winx, winy;
	
	with(self.document) {
		winx = documentElement.clientWidth || body.clientWidth || body.scrollWidth;
		winy = documentElement.clientHeight || body.clientHeight || body.scrollHeight;
	}
	
	var overlay = dom_get_id("overlay");
	if(overlay != null) with(overlay) {
		style.width = winx;
		style.height = winy;
	}
	
	var modal = dom_get_id("modal");
	if(modal != null) with(modal) {
		style.left = parseInt((winx - clientWidth) / 2, 10).toString() + "px";
		style.top = parseInt((winy - clientHeight) / 2, 10).toString() + "px";
	}
	
	return false;
}

// ダイアログを閉じる
function modal_hide() {
	if(dom_get_id("modal") != null) with(dom_get_tag("body")[0]) {
		// モーダルウィンドウを削除する
		removeChild(dom_get_id("modal"));
		
		// オーバーレイを削除する
		removeChild(dom_get_id("overlay"));
		
		// ウィンドウサイズが変更された場合、何もしない
		self.window.onresize = null;
	}
}

// データを取得する
function dom_get_storage(key, pass) {
	var enc, dec;
	
	if(typeof pass == "string") {
		// ローカルストレージ：復号する
		enc = self.window.localStorage.getItem(key);
		if(enc == null) {
			// 暗号化データが存在しない場合
			dec = enc;
		} else {
			// 暗号化データを復号する
			dec = CryptoJS.AES.decrypt(enc, pass);
			dec = (dec.toString().substring(0, (pass + "\t").length * 2) == CryptoJS.enc.Utf8.parse(pass + "\t")? dec.toString(CryptoJS.enc.Utf8): "");
			if(dec == "") {
				// 正しく復号できない場合
				dec = "";
			} else {
				dec = dec.substring((pass + "\t").length)
			}
		}
	} else {
		// セッションストレージ
		dec = self.window.sessionStorage.getItem(key);
	}
	return dec;
}

// データを設定する
function dom_set_storage(key, value, pass) {
	if(typeof pass == "string") {
		// ローカルストレージ：暗号化する
		self.window.localStorage.setItem(key, CryptoJS.AES.encrypt(((pass + "\t") + (value != ""? value: "\r\n")).toString(CryptoJS.enc.Utf8), pass));
	} else {
		// セッションストレージ
		self.window.sessionStorage.setItem(key, value);
	}
	return;
}

// データを削除する
function dom_del_storage(key, pass) {
	var i;
	
	if(typeof pass == "string") {
		// ローカルストレージ
		with(self.window) if(pass == "*") {
			for(i = localStorage.length - 1; i >= 0; i--) if(localStorage.key(i).indexOf(key) == 0) localStorage.removeItem(localStorage.key(i));
		} else {
			localStorage.removeItem(key);
		}
	} else {
		self.window.sessionStorage.removeItem(key);
	}
	return;
}

function fnc_getauth(rowid) {
	var logons = local_current();
	var auths = dom_get_storage(logons["localid"], logons["localpass"]).split("\r\n");
	var bufs = new Array();
	var auth = "";
	var i;
	
	var auth = undefined;
	for(i = 0; i < auths.length; i++) {
		bufs = auths[i].split("\t");
		if(rowid == bufs[0]) {
			auth = auths[i];
			break;
		}
	}
	
	return auth;
}

function auth_parse(auth) {
	var rets = new Array();
	var rownum = -1;
	var rowid = "";
	var fiid = "";
	var keyvalues = new Array();
	var lists = new Array();
	var i, j;
	
	// rownum、rowid、fiidを切り出す
	i = auth.indexOf("=");
	j = auth.indexOf("\t");
	if(j != -1) {
		if(i < j) {
			if(i > 0) rownum = parseInt(auth.substring(0, i), 10);
			rowid = auth.substring(0, j);
			fiid = auth.substring(i + 1, j);
		}
		auth = auth.substring(j + 1);
	}
	
	// keyvaluesを切り出す
	lists = auth.split("\t");
	for(i = 0; i < lists.length; i++) {
		j = lists[i].indexOf("=");
		if(j > 0) keyvalues[lists[i].substring(0, j)] = lists[i].substring(j + 1);
	}
	
	rets["rownum"] = rownum;
	rets["rowid"] = rowid;
	rets["fiid"] = fiid;
	rets["keyvalues"] = keyvalues;
	
	return rets;
}

function logoninfo_add(auth) {
	var logons = local_current();
	var lists = new Array();
	var settings;
	var enc;
	
	// ログオン情報を追加する
	enc = dom_get_storage(logons["localid"], logons["localpass"]);
	lists = enc.split("\r\n");
	lists.push(auth);
	lists = auths_sort(lists);
	
	// ログオン情報を記憶する
	dom_set_storage(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_listall(lists);
}

function logoninfo_update(to, from) {
	var logons = local_current();
	var lists = new Array();
	var enc;
	
	// ログオン情報を更新する
	enc = dom_get_storage(logons["localid"], logons["localpass"]);
	if(from != null && from.length != 0 && enc.indexOf(from) != -1) {
		enc = enc.replace(from, to);
		lists = enc.split("\r\n");
	}
	lists = auths_sort(lists);
	
	// ログオン情報を設定する
	dom_set_storage(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_listall(lists);
}

function logoninfo_delete(auth) {
	var logons = local_current();
	var lists = new Array();
	var settings = auth_parse(auth);
	var enc;
	
	// ログオン情報を削除する
	enc = dom_get_storage(logons["localid"], logons["localpass"]);
	if(enc.indexOf(auth) != -1) {
		enc = enc.replace(auth, "");
		lists = enc.split("\r\n");
		
		// OFXを削除する
		dom_del_storage(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
	}
	lists = auths_sort(lists);
	
	// ログオン情報を記憶する
	dom_set_storage(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_listall(lists);
}

function auths_sort(auths) {
	var logons = local_current();
	
	var rets = new Array();
	var settings;
	var rowid, auth, ofx;
	var i, j, k, l;
	
	// authsをfiids[]の登録順でソートする
	k = 0;
	l = -1;
	for(i in fiids) {
		for(j = 0; j < auths.length; j++) {
			settings = auth_parse(auths[j]);
			if(settings["fiid"] == i) {
				// fiidが一致した場合
				if(settings["rownum"] != k) {
					// ソート順が崩れた場合
					if(l == -1) l = k;
					rowid = (settings["rownum"] == -1? k.toString() + settings["rowid"]: settings["rowid"].replace(settings["rownum"].toString(), k.toString()));
					// if(settings["rownum"] == -1) alert(k.toString() + settings["rowid"]);
					auth = auths[j].replace(settings["rowid"], rowid);
				} else {
					auth = auths[j];
				}
				rets.push(auth);
				k++;
			}
		}
	}
	
	if(l != -1) {
		if(settings["rownum"] == -1) {
			// 追加の場合、OFXを後から前に送る
			for(k = rets.length - 1; k >= l; k--) {
				settings = auth_parse(rets[(k == rets.length - 1? k: k + 1)]);
				rowid = settings["rowid"].replace(settings["rownum"].toString(), k.toString());
				// alert("+" + rowid + ":" + settings["rowid"]);
				
				ofx = dom_get_storage(logons["localid"] + ":" + rowid, logons["localpass"]);
				if(ofx != null && ofx != "") {
					// OFXを削除・登録する
					dom_set_storage(logons["localid"] + ":" + settings["rowid"], ofx, logons["localpass"]);
					dom_del_storage(logons["localid"] + ":" + rowid, logons["localpass"]);
				}
			}
		} else {
			// 削除の場合、OFXを前から後に送る
			for(k = l; k <= rets.length; k++) {
				settings = auth_parse(rets[(k == 0? k: k - 1)]);
				rowid = settings["rowid"].replace(settings["rownum"].toString(), (k == 0? k + 1: k).toString());
				// alert("-" + rowid + ":" + settings["rowid"]);
				
				ofx = dom_get_storage(logons["localid"] + ":" + rowid, logons["localpass"]);
				if(ofx != null && ofx != "") {
					// OFXを削除・登録する
					dom_set_storage(logons["localid"] + ":" + settings["rowid"], ofx, logons["localpass"]);
					dom_del_storage(logons["localid"] + ":" + rowid, logons["localpass"]);
				}
			}
		}
	}
	
	return rets;
}

function account_total_update() {
	var total = 0;
	var table = dom_get_id("account");
	var tds = table.getElementsByTagName("td");
	var i;
	
	for(i = 0; i < tds.length; i++) if(tds[i].className == "balance" && tds[i].firstChild.nodeValue != "") total += parseInt(tds[i].firstChild.nodeValue.replace(/,/g, ""), 10);
	dom_get_id("total").firstChild.nodeValue = to_amount(total);
	
	return true;
}

// 現在のログオン情報を取得する
function local_current() {
	var rets = new Array();
	var inputs = fiids["local"]["form"].split(",");
	var i;
	
	for(i = 0; i < inputs.length; i++) rets[inputs[i]] = dom_get_storage(inputs[i]);
	
	return rets;
}

// 画面の未入力項目をチェックする
function form_empty_check() {
	var dis = false;
	var fiid, inputs;
	var i;
	
	if(dom_get_id("fiid") != null) {
		// 認証情報の場合
		fiid = dom_get_id("fiid").value;
		
		// 入力項目を取得する
		inputs = fiids[fiid]["form"].split(",");
		for(i = 0; i < inputs.length; i++) if(dom_get_id(inputs[i]).type != "hidden" && dom_get_id(inputs[i]).value == "") {
			// 未入力の場合
			dis = true;
			break;
		}
		if(dom_get_id("confirm") != null && dom_get_id("confirm").checked == false) dis = true;
		
		// 未入力項目がある場合はOKボタンをdisabledに設定する
		dom_get_id("modalok").disabled = dis;
	} else {
		// それ以外の場合
		
		// 入力項目を取得する
		inputs = dom_get_id("modal").getElementsByTagName("input");
		for(i = 0; i < inputs.length; i++) if(inputs[i].type != "hidden" && inputs[i].value == "") {
			// 未入力の場合
			dis = true;
			break;
		}
		if(dom_get_id("confirm") != null && dom_get_id("confirm").checked == false) dis = true;
		
		// 未入力項目がある場合はOKボタンをdisabledに設定する
		dom_get_id("modalok").disabled = dis;
	}
	
	return dis;
}


// =========================================================================
// 関数
// =========================================================================

function str_to_hankaku(str) {
	var f = function(str) {
		return String.fromCharCode(str.charCodeAt(0) - 0xFEE0);
	};
	
	return str.replace(/[！-～]/g, f).replace(/　/g," ");
}

function timestamp_get() {
	var dt = new Date();
	var y, m, d, h, i, s;
	with(dt) {
		y = getFullYear().toString();
		m = (getMonth() + 1).toString();
		d = getDate().toString();
		h = getHours().toString();
		i = getMinutes().toString();
		s = getSeconds().toString();
	}
	if(m.length == 1) m = "0" + m;
	if(d.length == 1) d = "0" + d;
	if(h.length == 1) h = "0" + h;
	if(i.length == 1) i = "0" + i;
	if(s.length == 1) s = "0" + s;
	
	return y + m + d + h + i + s;
}

function to_amount(src) {
	var dst = src.toString().replace(",", "");
	
	if(isNaN(Number(dst)) == true) {
		dst = "";
	} else {
		while(dst != (dst = dst.replace(/^(-?\d+)(\d{3})/, "$1,$2")));
	}
	
	return dst;
}

function chkenv_xmlhttprequest() {
	return (typeof XMLHttpRequest != "undefined" && XMLHttpRequest? true: false);
}

function chkenv_webstorage() {
	return (self.window.localStorage && self.window.sessionStorage? true: false);
}
function chkenv_domparser() {
	return (typeof DOMParser != "undefined" && DOMParser && typeof (new DOMParser).parseFromString == "function"? true: false);
}

function chkenv_xmlserializer() {
	return (typeof XMLSerializer != "undefined" && XMLSerializer && typeof (new XMLSerializer).serializeToString == "function"? true: false);
}

function chkenv_parser() {
	return chkenv_domparser() && chkenv_xmlserializer();
}

function chkenv_blob() {
	return (typeof Blob == "function" && Blob? true: false);
}

function chkenv_createobjecturl() {
	return ((self.window.URL || self.window.webkitURL) && ((self.window.URL || self.window.webkitURL).createObjectURL || self.window.navigator.msSaveOrOpenBlob)? true: false);
}

function chkenv_run() {
	return chkenv_xmlhttprequest() && chkenv_webstorage() && chkenv_parser();
}

function chkenv_ofx() {
	return chkenv_blob() && chkenv_createobjecturl();
}

function chkenv_ofx_all() {
	return chkenv_parser() && chkenv_blob() && chkenv_createobjecturl();
}

function dom_get_tag(name) {
	var obj = null;
	
	with(self.document) if(typeof getElementsByTagName != "undefined") try {
		if(typeof name == "string") obj = getElementsByTagName(name);
	} catch(e) {
		void(e);
	}
	
	return obj;
}

function dom_create_tag(name, attrs) {
	var obj = null;
	var i;
	
	with(self.document) if(typeof createElement != "undefined") try {
		if(typeof name == "string") obj = createElement(name);
	} catch(e) {
		void(e);
	}
	
	if(typeof attrs != "array") for(i in attrs) try {
		obj.setAttribute(i, dom_convert_escape(attrs[i]));
	} catch(e) {
		void(e);
	}
	
	return obj;
}

function dom_convert_escape(str) {
	var ret = "";
	var d, e, f;
	
	if(str.indexOf(String.fromCharCode(0x26)) == -1) {
		ret = str;
	} else {
		var hcs = {
		"amp": String.fromCharCode(0x26), 
		"quot": String.fromCharCode(0x22), 
		"lt": String.fromCharCode(0x3C), 
		"gt": String.fromCharCode(0x3E), 
		"nbsp": String.fromCharCode(0xA0), 
		"copy": String.fromCharCode(0xA9), 
		"reg": String.fromCharCode(0xAE)
		};
		f = function() {
			return hcs[arguments[1]];
		};
		e = "";
		for(d in hcs) e += "|" + d;
		ret = str.replace(new RegExp(hcs["amp"] + "(" + e.substr(1) + ");", "g"), f);
	}
	
	return ret;
}

function dom_get_id(name) {
	var obj = null;
	
	with(self.document) if(typeof getElementById != "undefined") try {
		if(typeof name == "string") obj = getElementById(name);
	} catch(e) {
		void(e);
	}
	
	return obj;
}

function dom_create_text(text) {
	var obj = null;
	
	with(self.document) if(typeof createTextNode != "undefined") try {
		if(typeof text == "string") obj = createTextNode(dom_convert_escape(text));
	} catch(e) {
		void(e);
	}
	
	return obj;
}
