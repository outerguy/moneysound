/*
template.js: 画面・機能を制御する
Copyright (C) 2014-2015 OFFICE OUTERGUY. All rights reserved.
mailto:contact@beatrek.com
Dual-licensed under the Apache License 2.0 and Beatrek Origin License.
*/

// 変数を定義する
var debug = "<!--[debug]-->";
var fprefix = "<!--[family]-->_";
var ver = "<!--[client]-->.<!--[server]-->";
var ofxhead = "<!--[ofxhead]-->";
var pdftext = "<!--[pdftext]-->";
var pdfdefsetting = "標準";
var get_all = -1;
var xhr = null;
var pw = false;
var px, py;
var fi;

// 連想配列を定義する
var ficats = { "BANK": "銀行", "CREDITCARD": "クレジットカード", "INVSTMT": "証券", "PREPAID": "前払式帳票" };
var themes = { "standard.css": "標準（スマートフォン対応）", "spartan.css": "Spartan（スマートフォン対応）", "modern.css": "Modern", "aero.css": "Aero", "luna.css": "Luna", "flat.css": "Flat", "aqua.css": "Aqua", "light.css": "Light", "precious.css": "プレシャス" };
var outputs = { "OFX": "OFXファイルの結合ダウンロード", "CSV": "CSVファイルのダウンロード", "PDF": "PDFファイルのダウンロード", "LPT": "口座一覧の印刷", "EXP": "口座情報のエクスポート" };
var ofxbuttons = { "T": "する", "F": "しない（出力ボタンの操作に追加する）" };
var csvencodings = { "SJIS": "Shift_JIS", "UTFB": "UTF-8（BOMあり）", "UTF8": "UTF-8（BOMなし）" };
var pdfrowpitchs = { "12": "狭い", "15": pdfdefsetting, "18": "広い" };
var fiids = "<!--[filist]-->";
fiids["logon"] = { "type": "LOCAL", "name": "ログオン", "form": "localid|localpass", "localid": "ローカルID|text", "localpass": "ローカルパスワード|password" };
fiids["register"] = { "type": "LOCAL", "name": "登録", "form": "localid|localpass", "localid": "ローカルID|text", "localpass": "ローカルパスワード|password" };
fiids["erase"] = { "type": "LOCAL", "name": "抹消", "form": "localid", "localid": "ローカルID|text" };

// 分類毎に金融機関の連想配列を生成する
var filists = new Array();
for(fi in ficats) filists[fi] = new Array();
for(fi in fiids) if(typeof filists[fiids[fi]["type"]] != "undefined") filists[fiids[fi]["type"]][fi] = fiids[fi];

(function() {
	// 起動時にロード機能を呼び出す
	with(self.window) if(addEventListener) {
		addEventListener("load", fnc_load, true);
	} else if(attachEvent) {
		attachEvent("onload", fnc_load);
	} else {
		onload = fnc_load;
	}
	
	with(self.document) {
		// キーに機能を割り当てる
		onkeydown = function(e) {
			var ret = true;
			var scroll = 10;
			
			if(typeof e == "undefined") e = self.window.event;
			if(e != null) switch(e.keyCode) {
			case 8: // BackSpaceキー
				if(dom_get_id("modal") != null && e.ctrlKey == true) {
					// モーダルウィンドウを表示していてCtrlキーが押下されている場合、モーダルウィンドウを中央に移動する
					modal_resize();
					ret = false;
				}
				break;
			case 13: // Enterキー
				if(dom_get_id("modal") != null && dom_get_id("modalok") != null && dom_get_id("modalok").disabled == false) {
					// モーダルウィンドウを表示している場合、OKボタンを割り当てる
					dom_get_id("modal").onsubmit();
				} else {
					// それ以外の場合、フォーカスのあるボタンを押下する
					with(activeElement) {
						try {
							onclick();
						} catch(e) {
							click();
						}
					}
				}
				ret = false;
				break;
			case 27: // Escキー
				if(dom_get_id("modal") != null) {
					// モーダルウィンドウを表示している場合、キャンセルボタンを割り当てる
					dom_get_id("modal").onreset();
				} else {
					// それ以外の場合、中止ボタンを割り当てる
					dom_get_id("btn_cancel").onclick();
				}
				ret = false;
				break;
			case 37: // ←キー
				if(dom_get_id("modal") != null && e.ctrlKey == true) {
					// モーダルウィンドウを表示していてCtrlキーが押下されている場合、モーダルウィンドウを移動する
					with(dom_get_id("modal").style) left = (parseInt(left.replace("px", ""), 10) - scroll).toString() + "px";
					ret = false;
				}
				break;
			case 38: // ↑キー
				if(dom_get_id("modal") != null && e.ctrlKey == true) {
					// モーダルウィンドウを表示していてCtrlキーが押下されている場合、モーダルウィンドウを移動する
					with(dom_get_id("modal").style) top = (parseInt(top.replace("px", ""), 10) - scroll).toString() + "px";
					ret = false;
				}
				break;
			case 39: // →キー
				if(dom_get_id("modal") != null && e.ctrlKey == true) {
					// モーダルウィンドウを表示していてCtrlキーが押下されている場合、モーダルウィンドウを移動する
					with(dom_get_id("modal").style) left = (parseInt(left.replace("px", ""), 10) + scroll).toString() + "px";
					ret = false;
				}
				break;
			case 40: // ↓キー
				if(dom_get_id("modal") != null && e.ctrlKey == true) {
					// モーダルウィンドウを表示していてCtrlキーが押下されている場合、モーダルウィンドウを移動する
					with(dom_get_id("modal").style) top = (parseInt(top.replace("px", ""), 10) + scroll).toString() + "px";
					ret = false;
				}
				break;
			default:
				break;
			}
			
			return ret;
		};
	}
})();


// =========================================================================
// 機能
// =========================================================================

// ロード機能
function fnc_load() {
	var fnc_btns = [fnc_logon, fnc_logoff, fnc_register, fnc_erase, fnc_debug, fnc_option, fnc_version, fnc_update_all, fnc_cancel, fnc_ofx_all, fnc_create, fnc_output];
	var tag_nav = dom_get_tag("nav")[0];
	var tag_as = dom_get_tag("a");
	var tag_p, tag_strong;
	var i;
	
	// デバッグ機能が有効の場合
	if(debug != false) {
		// 警告を表示する
		tag_p = dom_create_tag("p", { "id": "debug" });
		tag_strong = dom_create_tag("strong");
		tag_strong.appendChild(dom_create_text("【警告】開発者向け（デバッグ）機能が有効のため、認証情報を含む詳細な記録が残ります。開発者以外の方は、操作しないでください。または、開発者へご相談ください。"));
		tag_p.appendChild(tag_strong);
		tag_nav.parentNode.insertBefore(tag_p, tag_nav);
		
		// デバッグ情報ボタンを表示する
		dom_get_id("btn_debug").className = "";
	}
	
	// ボタンに機能を割り当てる
	for(i = 0; i < fnc_btns.length; i++) dom_get_id("btn" + fnc_btns[i].toString().replace(/^[^_]+(_[a-z_]+)[\w\W]+$/m, "$1")).onclick = fnc_btns[i];
	
	// リンク先を設定する
	for(i = 0; i < tag_as.length; i++) tag_as[i].target = "link";
	
	// 初期化機能を呼び出す
	fnc_initialize();
	
	return;
}

// 初期化機能
function fnc_initialize() {
	var tag_table = dom_get_tag("table")[0];
	var tag_caption = dom_get_tag("caption")[0];
	var btn_disableds;
	var tag_p;
	var logons, lists;
	var ofxbutton, inputs;
	var i;
	
	if(chkenv_run() == false) {
		tag_p = dom_create_tag("p", { "class": "ac" });
		tag_p.appendChild(dom_create_text("ご利用のブラウザーでは実行できません。"));
		
		tag_table.parentNode.replaceChild(tag_p, tag_table);
	} else {
		logons = local_current();
		lists = storage_get(logons["localid"], logons["localpass"]);
		switch(lists) {
		case null:
		case "":
			// ログオン情報を削除する
			for(i in logons) storage_delete(i);
			lists = "";
			
			// 表題を設定する
			tag_caption.firstChild.nodeValue = "ログオンしてください";
			
			// ボタンの有効・無効を指定する
			btn_disableds = { "btn_logon": false, "btn_logoff": true, "btn_register": false, "btn_erase": false, "btn_debug": true, "btn_option": true, "btn_version": false, "btn_update_all": true, "btn_cancel": true, "btn_ofx_all": true, "btn_create": true, "btn_output": true };
			
			break;
		default:
			// 表題を設定する
			tag_caption.firstChild.nodeValue = logons["localid"];
			
			// ボタンの有効・無効を指定する
			btn_disableds = { "btn_logon": true, "btn_logoff": false, "btn_register": true, "btn_erase": true, "btn_debug": false, "btn_option": false, "btn_version": false, "btn_update_all": true, "btn_cancel": true, "btn_ofx_all": true, "btn_create": false, "btn_output": true };
			
			break;
		}
		
		// ボタンを有効・無効に設定する
		for(i in btn_disableds) dom_get_id(i).disabled = btn_disableds[i];
		
		// 画面のテーマよりCSSを選択する
		var css = storage_get(logons["localid"] + ":theme", logons["localpass"]);
		if(css == null || css == "") for(i in themes) {
			css = i;
			break;
		}
		
		// CSSを制御する
		with(dom_get_id("css_theme")) href = href.substring(0, href.lastIndexOf("/") + 1) + css;
		
		// OFXボタンの表示を取得する
		ofxbutton = storage_get(logons["localid"] + ":ofxbutton", logons["localpass"]);
		if(ofxbutton == null) for(i in ofxbuttons) {
			ofxbutton = i;
			break;
		}
		
		// OFXボタンの表示を制御する
		dom_get_id("btn_ofx_all").style.display = (ofxbutton == "T"? "inline": "none");
		
		// 口座一覧を生成する
		fnc_list_all(lists.split("\r\n"));
	}
	
	return;
}

// ログオン機能
function fnc_logon() {
	var fiid = "logon";
	var cdf = document.createDocumentFragment();
	var auths = new Array();
	var f = false;
	var tag_p;
	var lists, inputs;
	var dec;
	var us, ps;
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
		inputs = fiids[fiid]["form"].split("|");
		for(i = 0; i < inputs.length; i++) {
			lists = fiids[fiid][inputs[i]].split("|", 2);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(lists[0]));
			cdf.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "value": (typeof auths[inputs[i]] == "string"? auths[inputs[i]]: ""), "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			cdf.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show(fiids[fiid]["name"], cdf, true, inputs[0]);
		
		// 未入力項目をチェックする
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
			inputs = fiids[fiid]["form"].split("|");
			auths.push("=" + fiid);
			for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
			modal_hide();
			
			us = auths[1].split("=", 2);
			ps = auths[2].split("=", 2);
			
			// 暗号化データを取得する
			dec = storage_get(us[1], ps[1]);
			switch(dec) {
			case null:
				// 暗号化データが存在しない場合、エラー画面を表示する
				modal_show("エラー", "正しい" + fiids[fiid][us[0]].split("|", 2)[0] + "を入力してください。", false);
				break;
			case "":
				// 正しく復号できない場合、エラー画面を表示する
				modal_show("エラー", "正しい" + fiids[fiid][ps[0]].split("|", 2)[0] + "を入力してください。", false);
				break;
			default:
				// ログオン情報を設定する
				storage_set(us[0], us[1]);
				storage_set(ps[0], ps[1]);
				
				// 初期化機能を呼び出す
				fnc_initialize();
				
				break;
			}
		}
	}
	
	return;
}

// ログオフ機能
function fnc_logoff() {
	var logons = local_current();
	var i;
	
	// ログオン情報を削除する
	for(i in logons) storage_delete(i);
	
	// 初期化機能を呼び出す
	fnc_initialize();
	
	return;
}

// 登録機能
function fnc_register() {
	var fiid = "register";
	var cdf = document.createDocumentFragment();
	var auths = new Array();
	var tag_p;
	var inputs, txt, dec;
	var us, ps;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 入力項目を設定する
		inputs = fiids[fiid]["form"].split("|");
		for(i = 0; i < inputs.length; i++) {
			lists = fiids[fiid][inputs[i]].split("|", 2);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text(lists[0]));
			cdf.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
			cdf.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("口座情報のインポート（オプション）"));
		cdf.appendChild(tag_p);
		
		if(chkenv_import() == true) {
			// ファイルアップロードを表示する
			tag_p = dom_create_tag("p");
			tag_input = dom_create_tag("input", { "type": "file", "name": "enc", "id": "enc", "accept": "text/plain", "onchange": "with(new FileReader()) { readAsText(this.files[0]); onload = function() { dom_get_id(\"txt\").value = this.result; }; }" });
			tag_p.appendChild(tag_input);
			cdf.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_input = dom_create_tag("input", { "type": "hidden", "name": "txt", "id": "txt" });
			tag_p.appendChild(tag_input);
			cdf.appendChild(tag_p);
		} else {
			// テキストボックスを表示する
			tag_p = dom_create_tag("p");
			tag_input = dom_create_tag("input", { "type": "text", "name": "txt", "id": "txt" });
			tag_p.appendChild(tag_input);
			cdf.appendChild(tag_p);
		}
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show(fiids[fiid]["name"], cdf, true, inputs[0]);
		
		// 未入力項目をチェックする
		form_empty_check();
	} else {
		// コールバックの場合
		if(dom_get_id("fiid") == null) {
			// エラー画面を表示した後の場合、登録画面を表示する
			modal_hide();
			fnc_register();
		} else {
			// ログオン情報を生成する
			txt = dom_get_id("txt").value;
			fiid = dom_get_id("fiid").value;
			inputs = fiids[fiid]["form"].split("|");
			auths.push("=" + fiid);
			for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
			
			// モーダルウィンドウを閉じる
			modal_hide();
			
			us = auths[1].split("=", 2);
			ps = auths[2].split("=", 2);
			
			// 口座情報を復号する
			try {
				dec = CryptoJS.AES.decrypt(txt.split(",")[1], ps[1]);
				if(dec != "") {
					dec = (dec.toString().substring(0, (ps[1] + "\t").length * 2) == CryptoJS.enc.Utf8.parse(ps[1] + "\t")? dec.toString(CryptoJS.enc.Utf8): "");
					dec = (dec == ""? "": dec.substring((ps[1] + "\t").length));
				}
			} catch(e) {
				dec = "";
			}
			
			if(txt != "" && dec == "") {
				// 口座情報が指定されていて、かつ復号結果が空の場合、エラーを表示する
				modal_show("エラー", "口座情報が正しくないか、または" + fiids["logon"][ps[0]].split("|", 2)[0] + "が一致しません。", false);
			} else {
				switch(storage_get(us[1], ps[1])) {
				case null:
					// ログオン情報を設定する
					storage_set(us[1], dec, ps[1]);
					modal_showonly("完了", us[1] + "を登録しました。ログオンしてください。", false);
					break;
				case "":
				default:
					modal_show("エラー", us[1] + "は既に存在しています。", false);
					break;
				}
			}
		}
	}
	
	return;
}

// 抹消機能
function fnc_erase() {
	var fiid = "erase";
	var cdf = document.createDocumentFragment();
	var tag_p, tag_label;
	var input, key;
	
	if(dom_get_id("modal") == null) {
		// 入力項目を設定する
		input = fiids[fiid]["form"].split("|")[0];
		lists = fiids[fiid][input].split("|", 2);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text(lists[0]));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": input, "id": input, "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("確認"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "checkbox", "name": "confirm", "id": "confirm", "value": "", "onclick": "form_empty_check();" }));
		tag_label = dom_create_tag("label", { "for": "confirm" });
		tag_label.appendChild(dom_create_text("抹消する（元に戻せません）"));
		tag_p.appendChild(tag_label);
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show(fiids[fiid]["name"], cdf, true, input);
		
		// 未入力項目をチェックする
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
			input = fiids[fiid]["form"].split("|")[0];
			key = dom_get_id(input).value;
			
			// モーダルウィンドウを閉じる
			modal_hide();
			
			switch(storage_get(key, "")) {
			case null:
				modal_show("エラー", key + "は存在しません。", false);
				break;
			case "":
			default:
				// ログオン情報、およびOFXを削除する
				storage_delete(key, "*");
				modal_showonly("完了", key + "を抹消しました。", false);
				break;
			}
		}
	}
	
	return;
}

// デバッグ情報機能
function fnc_debug() {
	var logons = local_current();
	var auth = storage_get(logons["localid"], logons["localpass"]);
	var cdf = document.createDocumentFragment();
	var tag_div = dom_create_tag("div", { "id": "details" });
	var tag_pre = dom_create_tag("pre");
	
	tag_pre.appendChild(dom_create_text(auth.replace(/\t/g, " ")));
	tag_div.appendChild(tag_pre);
	cdf.appendChild(tag_div);
	
	// モーダルウィンドウを開く
	modal_showonly("デバッグ情報", cdf, false);
	
	return;
}

// 設定機能
function fnc_option() {
	var logons = local_current();
	var cdf = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var css, ofxbutton, csvencoding, pdfrowpitch;
	var i;
	
	// OFXボタンの表示を取得する
	ofxbutton = storage_get(logons["localid"] + ":ofxbutton", logons["localpass"]);
	if(ofxbutton == null) for(i in ofxbuttons) {
		ofxbutton = i;
		break;
	}
	
	// CSVファイルの文字エンコーディングを取得する
	csvencoding = storage_get(logons["localid"] + ":csvencoding", logons["localpass"]);
	if(csvencoding == null) for(i in csvencodings) {
		csvencoding = i;
		break;
	}
	
	// PDFファイルの行ピッチを取得する
	pdfrowpitch = storage_get(logons["localid"] + ":pdfrowpitch", logons["localpass"]);
	if(pdfrowpitch == null) for(i in pdfrowpitchs) if(pdfrowpitchs[i] == pdfdefsetting) {
		pdfrowpitch = i;
		break;
	}
	
	if(dom_get_id("modal") == null) {
		// 画面のテーマリストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("画面のテーマ"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "theme", "id": "theme" });
		for(i in themes) {
			tag_option = dom_create_tag("option", { "value": i });
			if(dom_get_id("css_theme").href.indexOf(i) != -1) tag_option["selected"] = "selected";
			tag_option.appendChild(dom_create_text(themes[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// OFXボタンの表示リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("OFXボタンの表示"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "ofxbutton", "id": "ofxbutton" });
		for(i in ofxbuttons) {
			tag_option = dom_create_tag("option", { "value": i });
			if(i == ofxbutton) tag_option["selected"] = "selected";
			tag_option.appendChild(dom_create_text(ofxbuttons[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// CSVファイルの文字エンコーディングリストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("CSVファイルの文字エンコーディング"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "csvencoding", "id": "csvencoding" });
		for(i in csvencodings) {
			tag_option = dom_create_tag("option", { "value": i });
			if(i == csvencoding) tag_option["selected"] = "selected";
			tag_option.appendChild(dom_create_text(csvencodings[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// PDFファイルの行ピッチリストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("PDFファイルの行ピッチ"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "pdfrowpitch", "id": "pdfrowpitch" });
		for(i in pdfrowpitchs) {
			tag_option = dom_create_tag("option", { "value": i });
			if(i == pdfrowpitch) tag_option["selected"] = "selected";
			tag_option.appendChild(dom_create_text(pdfrowpitchs[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show("設定", cdf, true, "theme");
	} else {
		// コールバックの場合
		css = dom_get_id("theme")[dom_get_id("theme").selectedIndex].value;
		ofxbutton = dom_get_id("ofxbutton")[dom_get_id("ofxbutton").selectedIndex].value;
		csvencoding = dom_get_id("csvencoding")[dom_get_id("csvencoding").selectedIndex].value;
		pdfrowpitch = dom_get_id("pdfrowpitch")[dom_get_id("pdfrowpitch").selectedIndex].value;
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		// 画面のテーマを設定する
		storage_set(logons["localid"] + ":theme", css, logons["localpass"]);
		
		// OFXボタンの表示を設定する
		storage_set(logons["localid"] + ":ofxbutton", ofxbutton, logons["localpass"]);
		
		// CSVファイルの文字エンコーディングを設定する
		storage_set(logons["localid"] + ":csvencoding", csvencoding, logons["localpass"]);
		
		// PDFファイルの行ピッチを設定する
		storage_set(logons["localid"] + ":pdfrowpitch", pdfrowpitch, logons["localpass"]);
		
		// 初期化機能を呼び出す
		fnc_initialize();
	}
	
	return;
}

// バージョン情報機能
function fnc_version() {
	var cdf = document.createDocumentFragment();
	var title = dom_get_tag("title")[0].firstChild.nodeValue;
	var img_icon = "data:image/gif;base64,R0lGODlhIAAgAPIAAP///wCZ/xqg+0Wy+2G9+5HR/Lnh/Pv8/SH5BAEAAAAALAAAAAAgACAAAAOVCArR/jC6taS9r+JtFf8Q4wykIAnkADrEcRRS4RKrY7jmI7hGPbqwh+yg8jVuh1xgd+gZGwOgY1h8BpAmptO6lA6V3GFAyn0AXeWlYI1srtfPtms+39bk9PrzzXbz01dNgHwCNwaEcXmKdit4i3t8hohphoAQlRcUHhyYEZqaGwMEVRmfoEampj6pqjWsrR+vrLGyCwkAOw==";
	var img_wsofx = "data:image/gif;base64,R0lGODlhWAAfAPQAAAAAAAAAAAUFBQ0NDRwcHDMzM0BAQFNTUwGdNGlpaf8A/xilRgCczyOoUROh0TKuWoKCgiWp1TKr1k68c5ubm0u637KysnfH44bPn8DAwKnj7MPm0tPt3c/s9uXp5/Pz8yH5BAEAAAoALAAAAABYAB8AAAX+oCKOZGmeaKqubOuOCPLOdG0r8a3vfE4HCiDwFCjyhjyY7BdsBpHIptD4hA6LWFunYvKRJB2VUCqCjsZX55jsnG0ZjO6SBAenstVoes1X+19vDA4XciYaEXVhJn1EZHtlfmkuFXCDIhwTDRsiPhsPD5sKh3WLbVRVZ1NXp1SrLnAMdqINMZoxnjELGCKBcUltwElwiLGKGrQxybq8lIJwv8BRwnGjxSLHubsKgZbPKBkQ4RAZ0DbeosSyGtrchCLnJBQEWFgEFCT0+UUQ9AcjBVgE3GPhLVQ1We0uvfNFIkERAfQgBkiQSl+WA/TIUXg48ZUDTqCupWtmSQEmBBz+FCAqwc8iPQhlXFIZgKWAgnlFCLxwpwBZSHSV3J1EsMDDtRI0AwgYMJDCAIkDYhZJIE6ciJZFAGKxcAMbgp/ehi5DsRHLQBEW6N3LmEJrPn87MCRbyCnGgw8qHOY04ZYi228SsQwgd2OoD2/JEGhKgbEIXBKNAxjIU8CAZcsl9CoNcJaGWLlLECPAgGyxCQNYHo/QPFmmtCpLbXyuSxcHylm1Qo2IrFpE5NYyS0TeN2O2ktoxUl4rrVuzzhJ981CtSiJtPgGEWzwgqo3EYV/JS5BWPKJskc7Wz+fJbgJnvskuPHQvcaz2WBMYNuAVkXRp06dFRLUeClhJthln5ShmAI8LBboE04AmZBDYZDgtxR4NHUggizdbRKBbXhwFNFVFAVwoglvYKZBeAL3NcFAHcCTUgjz62IMPFiaa15EIqJm1QzWwCMLTC+CIY2JVJhYZDktGJlFNSQlG+YsGQ0pp5ZVYJhECADs=";
	var fnc_chkenvs = [chkenv_xmlhttprequest, chkenv_webstorage, chkenv_domparser, chkenv_xmlserializer, chkenv_blob, chkenv_createobjecturl, chkenv_arraybuffer, chkenv_filereader];
	var f = true;
	var unsupports = new Array();
	var tag_p, tag_a, tag_img, tag_hr;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 表示項目を設定する
		tag_p = dom_create_tag("p");
		tag_img = dom_create_tag("img", { "src": img_wsofx, "width": "88", "height": "31", "alt": "We Support OFX", "style": "float: right; clear: right;" });
		tag_p.appendChild(tag_img);
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_img = dom_create_tag("img", { "src": img_icon, "width": "32", "height": "32", "alt": title + "アイコン", "style": "float: left; clear: left; margin-right: 1em;" });
		tag_p.appendChild(tag_img);
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "style": "margin-bottom: 8px; line-height: 32px; font-weight: bold;" });
		tag_a = dom_create_tag("a", { "href": "https://github.com/outerguy/moneysound", "target": "_blank", "style": "margin-right: 0.5em;" });
		tag_a.appendChild(dom_create_text(title));
		tag_p.appendChild(tag_a);
		tag_p.appendChild(dom_create_text("Version " + ver));
		cdf.appendChild(tag_p);
		
		tag_hr = dom_create_tag("hr");
		cdf.appendChild(tag_hr);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text("Copyright (C) 2008-2015 OFFICE OUTERGUY. All rights reserved."));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text("Portions Copyright (C) 2012-2015 Hiromu2000. All rights reserved."));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("使用しているライブラリー"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_a = dom_create_tag("a", { "href": "http://code.google.com/p/crypto-js/", "target": "_blank", "style": "margin-right: 0.5em;" });
		tag_a.appendChild(dom_create_text("CryptoJS v3.1.2"));
		tag_p.appendChild(tag_a);
		tag_p.appendChild(dom_create_text("(c) 2009-2013 by Jeff Mott. All rights reserved."));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_a = dom_create_tag("a", { "href": "https://github.com/polygonplanet/encoding.js", "target": "_blank", "style": "margin-right: 0.5em;" });
		tag_a.appendChild(dom_create_text("Encoding.js version 1.0.23"));
		tag_p.appendChild(tag_a);
		tag_p.appendChild(dom_create_text("Copyright (c) 2013-2015 polygon planet"));
		cdf.appendChild(tag_p);
		
		with(unsupports) {
			for(i = 0; i < fnc_chkenvs.length; i++) if(fnc_chkenvs[i]() == false) push(fnc_chkenvs[i].toString().replace(/^[^_]+_([a-z_]+)[\w\W]+$/m, "$1"));
			if(length > 0) {
				tag_p = dom_create_tag("p", { "class": "label" });
				tag_p.appendChild(dom_create_text("ご利用のブラウザーが対応していない機能"));
				cdf.appendChild(tag_p);
				
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_text(join(" ")));
				cdf.appendChild(tag_p);
			}
		}
		
		// モーダルウィンドウを開く
		modal_show("バージョン情報", cdf, false);
	} else {
		// モーダルウィンドウを閉じる
		modal_hide();
	}
	
	return;
}

// 追加機能
function fnc_create() {
	var cdf = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var fiid;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 分類リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("分類"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "ficat", "id": "ficat", "onchange": "fnc_create_change(this[this.selectedIndex].value);", "onkeyup": "this.onchange();" });
		for(i in ficats) {
			tag_option = dom_create_tag("option", { "value": i });
			tag_option.appendChild(dom_create_text(ficats[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// 金融機関リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("金融機関"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "fiid", "id": "fiid", "size": "8", "ondblclick": "dom_get_id(\"modal\").onsubmit();" });
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show("追加", cdf, true, "ficat");
		
		// 分類リストの先頭を選択する
		fnc_create_change();
	} else {
		// コールバックの場合
		fiid = dom_get_id("fiid").options[dom_get_id("fiid").selectedIndex].value;
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		// 変更画面を表示する
		fnc_modify("=" + fiid);
	}
	
	return;
}

// 分類を変更した場合、金融機関リストを更新する
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
	
	return;
}

// 変更機能
function fnc_modify(rowid) {
	var logons = local_current();
	var cdf = document.createDocumentFragment();
	var auths = new Array();
	var auth = auth_get(rowid);
	var inactive = false;
	var lists, inputs, settings;
	var fiid;
	var tag_p;
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
		if(typeof fiids[fiid]["form"] == "undefined") {
			// 未定義の金融機関の場合、スキップする
			inactive = true;
		} else {
			// 入力項目を設定する
			inputs = fiids[fiid]["form"].split("|");
			for(i = 0; i < inputs.length; i++) {
				lists = fiids[fiid][inputs[i]].split("|", 2);
				
				tag_p = dom_create_tag("p", { "class": "label" });
				tag_p.appendChild(dom_create_text(lists[0]));
				cdf.appendChild(tag_p);
				
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_tag("input", { "type": lists[1], "name": inputs[i], "id": inputs[i], "value": (typeof auths[inputs[i]] == "string"? auths[inputs[i]]: ""), "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
				cdf.appendChild(tag_p);
			}
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "fiid", "id": "fiid", "value": fiid }));
			if(typeof auth == "string") tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
			cdf.appendChild(tag_p);
			
			// モーダルウィンドウを開く
			modal_show(fiids[fiid]["name"], cdf, true, inputs[0]);
			
			// 未入力項目をチェックする
			form_empty_check();
		}
	} else {
		// 認証情報を生成する
		fiid = dom_get_id("fiid").value;
		auth = (dom_get_id("auth") != null? dom_get_id("auth").value: null);
		inputs = fiids[fiid]["form"].split("|");
		auths.push("=" + fiid);
		for(i = 0; i < inputs.length; i++) auths.push(dom_get_id(inputs[i]).id + "=" + dom_get_id(inputs[i]).value);
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		if(auth != null) {
			settings = auth_parse(auth);
			if(settings["rownum"] != -1) {
				// OFXを削除する
				storage_delete(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
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
	
	return;
}

// 削除機能
function fnc_delete(rowid) {
	var cdf = document.createDocumentFragment();
	var auth;
	var fiid;
	var tag_p;
	
	if(dom_get_id("auth") == null) {
		auth = auth_get(rowid);
		fiid = rowid.split("=")[1];
		tag_p = dom_create_tag("p");
		tag_p.appendChild(dom_create_text(fiids[fiid]["name"] + "を削除します。"));
		tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show("削除", cdf, true, "modalcancel");
	} else {
		// コールバックの場合
		if(typeof auth != "string") auth = dom_get_id("auth").value;
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		logoninfo_delete(auth);
	}
	
	return;
}

// 出力機能
function fnc_output() {
	var logons = local_current();
	var cdf = document.createDocumentFragment();
	var tag_p, tag_select, tag_option;
	var output;
	var ofxbutton;
	var i;
	
	if(dom_get_id("modal") == null) {
		// 操作リストを生成する
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text("操作"));
		cdf.appendChild(tag_p);
		
		// OFXボタンの表示を取得する
		ofxbutton = storage_get(logons["localid"] + ":ofxbutton", logons["localpass"]);
		if(ofxbutton == null) for(i in ofxbuttons) {
			ofxbutton = i;
			break;
		}
		
		tag_p = dom_create_tag("p");
		tag_select = dom_create_tag("select", { "name": "ficat", "id": "format" });
		for(i in outputs) {
			// OFXボタンが表示されている場合、OFX結合ダウンロード機能を取り除く
			if(i == "OFX" && ofxbutton != "F") continue;
			tag_option = dom_create_tag("option", { "value": i });
			tag_option.appendChild(dom_create_text(outputs[i]));
			tag_select.appendChild(tag_option);
		}
		tag_p.appendChild(tag_select);
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show("出力", cdf, true, "format");
	} else {
		// コールバックの場合
		output = dom_get_id("format").options[dom_get_id("format").selectedIndex].value;
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		switch(output) {
		case "OFX":
			// OFX結合ダウンロード機能を呼び出す
			fnc_ofx_all();
			break;
		case "CSV":
			// CSVダウンロード機能を呼び出す
			fnc_csv();
			break;
		case "PDF":
			// PDFダウンロード機能を呼び出す
			fnc_pdf();
			break;
		case "LPT":
			// 口座一覧印刷機能を呼び出す
			fnc_print();
			break;
		case "EXP":
			// 口座情報エクスポート機能を呼び出す
			fnc_export();
			break;
		default:
			break;
		}
	}
	
	return;
}

// すべて更新機能
function fnc_update_all(auth) {
	var logons = local_current();
	var auths = storage_get(logons["localid"], logons["localpass"]).split("\r\n");
	var rowid = (typeof auth != "string" || auth.indexOf("=") == -1? 0: parseInt(auth.substring(0, auth.indexOf("=")), 10) + 1);
	var settings;
	
	if(typeof auths[rowid] != "undefined") {
		settings = auth_parse(auths[rowid]);
		get_all = rowid;
		fnc_update(settings["rowid"]);
	} else {
		get_all = -1;
	}
	
	return;
}

// 更新機能
function fnc_update(rowid, additional) {
	var auth = auth_get(rowid);
	var auths = auth.split("\t");
	var tag_html = dom_get_tag("html")[0];
	var querys = new Array();
	var inactive = false;
	var token = "";
	var btn_disableds = { "btn_logoff": true, "btn_debug": true, "btn_option": true, "btn_version": true, "btn_cancel": false, "btn_create": true, "btn_output": true };
	var fiid;
	var query, status;
	var inputs;
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
			
			if(typeof fiids[fiid]["form"] == "undefined") {
				// 未定義の金融機関の場合、スキップする
				inactive = true;
			} else {
				// 認証情報を展開する
				if(i == 0 || fiids[fiid]["form"].indexOf(k) != -1) querys.push(k + "=" + encodeURIComponent(l));
				if(k == "status") status = l;
				if(k == "token") token = l;
			}
		}
	}
	
	if(inactive == true) {
		get_all = -1;
	} else {
		if(status == "202" && typeof additional == "undefined") {
			// 追加認証の場合、追加認証機能を呼び出す
			fnc_update_additional(auth);
		} else {
			// 認証情報を結合・分離する
			if(token != "") querys.push("X-Token=" + token);
			if(typeof additional != "undefined") querys.push(additional);
			query = querys.join("&");
			querys[0] = m + "=" + fiid;
			if(typeof additional != "undefined") querys.pop();
			if(token != "") querys.pop();
			
			if(chkenv_xmlhttprequest() == true) {
				xhr = new XMLHttpRequest();
				with(xhr) {
					onreadystatechange = function() {
						var tag_html = dom_get_tag("html")[0];
						var tag_table = dom_get_tag("table")[0];
						var btn_disableds = { "btn_logoff": false, "btn_debug": false, "btn_option": false, "btn_version": false, "btn_cancel": true, "btn_create": false, "btn_output": false };
						var logons, ofx, inputs, query;
						var i;
						
						if(xhr != null && xhr.readyState == 4) {
							var logons = local_current();
							
							if(xhr.status != 0 && xhr.status != 204) {
								// OFXを設定する
								ofx = xhr.responseText;
								storage_set(logons["localid"] + ":" + querys[0], xhr.responseText, logons["localpass"]);
							}
							
							// 更新・明細・OFX・変更・削除ボタンの押下を許可する
							inputs = dom_get_tag("input", tag_table);
							for(i = 0; i < inputs.length; i++) switch(inputs[i].value) {
							case "更新":
							case "明細":
							case "OFX":
							case "変更":
							case "削除":
								inputs[i].disabled = false;
								break;
							default:
								break;
							}
							
							// ボタンを有効・無効に設定する
							for(i in btn_disableds) dom_get_id(i).disabled = btn_disableds[i];
							
							tag_html.className = "";
							dom_get_id(auths[0]).className = "";
							
							// 口座一覧の項目を更新する
							if(xhr.status != 0) {
								querys.push("status=" + xhr.status.toString());
								querys.push("timestamp=" + timestamp_get());
								
								if(xhr.getResponseHeader("X-Token") != null && xhr.getResponseHeader("X-Token") != "") token = xhr.getResponseHeader("X-Token");
								if(token != null && token != "") {
									i = token.indexOf(",");
									if(i != -1) token = token.substring(0, i);
									querys.push("token=" + token);
								}
								
								query = decodeURIComponent(querys.join("\t"));
								
								logoninfo_update(query, auth);
							} else {
								fnc_initialize();
							}
							
							// すべて更新機能を実行中の場合
							if(get_all != -1) fnc_update_all(auths[0]);
						}
						
						return;
					};
					open("POST", "./server.php?fiid=" + fiid, true);
					setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
					send(query);
				}
			}
			
			// 更新・明細・OFX・変更・削除ボタンの押下を禁止する
			inputs = dom_get_tag("input", tag_html);
			for(i = 0; i < inputs.length; i++) switch(inputs[i].value) {
			case "更新":
			case "明細":
			case "OFX":
			case "変更":
			case "削除":
				inputs[i].disabled = true;
				break;
			default:
				break;
			}
			
			// ボタンを有効・無効に設定する
			for(i in btn_disableds) dom_get_id(i).disabled = btn_disableds[i];
			
			tag_html.className = "pending";
			dom_get_id(auths[0]).className = "pending";
		}
	}
	
	return;
}

// 追加認証機能
function fnc_update_additional(auth) {
	var cdf = document.createDocumentFragment();
	var querys = new Array();
	var group = "";
	var status = "";
	var timestamp = "";
	var sesscookie = "";
	var accesskey = "";
	var mfaphraseid = "";
	var mfaphraselabel = "";
	var ofx = null;
	var parser = null;
	var auths;
	var tag_p;
	var logons, settings, str, inputs, query;
	var i, j, k, l;
	
	if(chkenv_parser() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、追加認証に対応していません。", false);
	} else {
		parser = new DOMParser();
		
		if(dom_get_id("modal") == null) {
			logons = local_current();
			settings = auth_parse(auth);
			str = storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			auths = auth.split("\t");
			
			switch(str) {
			case null:
			case "":
				break;
			default:
				if(parser != null) ofx = parser.parseFromString(str, "text/xml");
				break;
			}
			
			// 追加認証情報を取得する
			if(ofx != null) {
				sesscookie = dom_get_tag("SESSCOOKIE", ofx)[0].firstChild.nodeValue;
				accesskey = dom_get_tag("ACCESSKEY", ofx)[0].firstChild.nodeValue;
				mfaphraseid = dom_get_tag("MFAPHRASEID", ofx)[0].firstChild.nodeValue;
				mfaphraselabel = dom_get_tag("MFAPHRASELABEL", ofx)[0].firstChild.nodeValue;
			}
			
			// 追加認証情報を生成する
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth }));
			tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "additional", "id": "additional", "value": mfaphraseid }));
			tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "sesscookie", "id": "sesscookie", "value": sesscookie }));
			tag_p.appendChild(dom_create_tag("input", { "type": "hidden", "name": "accesskey", "id": "accesskey", "value": accesskey }));
			cdf.appendChild(tag_p);
			
			inputs = fiids[settings["fiid"]]["additional"].split("|");
			
			tag_p = dom_create_tag("p");
			tag_p.appendChild((inputs[2] == "image"? dom_create_tag("img", { "src": mfaphraselabel, "alt": "画像" }): dom_create_text(mfaphraselabel)));
			cdf.appendChild(tag_p);
			
			if(inputs[1] != "hidden") {
				tag_p = dom_create_tag("p", { "class": "label" });
				tag_p.appendChild(dom_create_text(inputs[0]));
				cdf.appendChild(tag_p);
				
				// 入力項目を設定する
				tag_p = dom_create_tag("p");
				tag_p.appendChild(dom_create_tag("input", { "type": inputs[1], "name": mfaphraseid, "id": mfaphraseid, "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" }));
				cdf.appendChild(tag_p);
			}
			
			// モーダルウィンドウを開く
			modal_show("追加認証", cdf, true, (inputs[1] != "hidden"? mfaphraseid: "modalok"));
			
			// 未入力項目をチェックする
			form_empty_check();
		} else {
			// コールバックの場合
			auth = dom_get_id("auth").value;
			
			// 追加認証情報を生成する
			querys.push("sesscookie=" + dom_get_id("sesscookie").value);
			querys.push("accesskey=" + dom_get_id("accesskey").value);
			if(dom_get_id(dom_get_id("additional").value) != null) querys.push(dom_get_id("additional").value + "=" + encodeURIComponent(dom_get_id(dom_get_id("additional").value).value));
			query = querys.join("&");
			
			// モーダルウィンドウを閉じる
			modal_hide();
			
			settings = auth_parse(auth);
			
			// 更新機能を呼び出す
			fnc_update(settings["rowid"], query);
		}
	}
	
	return;
}

// 中止機能
function fnc_cancel() {
	get_all = -1;
	xhr.abort();
	
	// 中止ボタンを無効に設定する
	dom_get_id("btn_cancel").disabled = true;
	
	return;
}

// 明細機能
function fnc_detail(rowid) {
	var cdf = document.createDocumentFragment();
	var tag_p, tag_select, tag_option, tag_div, tag_table, tag_thead, tag_tbody, tag_tr, tag_th, tag_td;
	var tag_seclists, tag_stmttrnrss, tag_stmttrns, tag_invtrans, tag_secids, tag_totals, tag_invposlists;
	var parser = null;
	var securitys = new Array();
	var auth, logons, settings, str, current;
	var uniqueidtypes, uniqueids, secnames, acctid, accttype, mktginfo, dtposted, name, trnamt, dttrade, security, total, dtpriceasofs, mktvals;
	var i, j;
	
	if(chkenv_parser() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、明細の表示に対応していません。", false);
	} else {
		parser = new DOMParser();
		
		if(dom_get_id("modal") == null) {
			auth = auth_get(rowid);
			logons = local_current();
			settings = auth_parse(auth);
			
			str = storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			if(str != null && str != "") {
				current = parser.parseFromString(str, "text/xml");
				
				// 証券一覧を生成する
				tag_seclists = dom_get_tag("SECLIST", current);
				if(tag_seclists.length > 0) {
					uniqueidtypes = dom_get_tag("UNIQUEIDTYPE", tag_seclists[0]);
					uniqueids = dom_get_tag("UNIQUEID", tag_seclists[0]);
					secnames = dom_get_tag("SECNAME", tag_seclists[0]);
					for(i = 0; i < secnames.length; i++) securitys[uniqueidtypes[i].firstChild.nodeValue + " " + uniqueids[i].firstChild.nodeValue] = secnames[i].firstChild.nodeValue;
				}
				
				// 口座種目を生成する
				tag_p = dom_create_tag("p", { "class": "label" });
				tag_p.appendChild(dom_create_text("口座種目"));
				cdf.appendChild(tag_p);
				
				tag_stmttrnrss = dom_get_tag("STMTTRNRS", current);
				if(tag_stmttrnrss.length == 0) tag_stmttrnrss = dom_get_tag("CCSTMTTRNRS", current);
				if(tag_stmttrnrss.length == 0) tag_stmttrnrss = dom_get_tag("INVSTMTTRNRS", current);
				
				if(tag_stmttrnrss.length == 0) {
					modal_showonly("警告", "表示可能な明細がありません。", false);
				} else {
					tag_select = dom_create_tag("select", { "id": "acct", "onchange": "fnc_detail_change();" });
					for(i = 0; i < tag_stmttrnrss.length; i++) {
						acctid = (dom_get_tag("ACCTID", tag_stmttrnrss[i]).length == 0? "": dom_get_tag("ACCTID", tag_stmttrnrss[i])[0].firstChild.nodeValue);
						accttype = (dom_get_tag("ACCTTYPE", tag_stmttrnrss[i]).length == 0? "": dom_get_tag("ACCTTYPE", tag_stmttrnrss[i])[0].firstChild.nodeValue);
						mktginfo = (dom_get_tag("MKTGINFO", tag_stmttrnrss[i]).length == 0? "": dom_get_tag("MKTGINFO", tag_stmttrnrss[i])[0].firstChild.nodeValue);
						j = mktginfo.indexOf("　");
						group = (accttype == "SAVINGS"? (j == -1? "預金": mktginfo.substring(j + 1)) + " " + acctid: acctid);
						
						tag_option = dom_create_tag("option", { "value": i.toString() });
						tag_option.appendChild(dom_create_text(utf8_zentohan(group)));
						tag_select.appendChild(tag_option);
					}
					
					cdf.appendChild(tag_select);
					
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
						tag_stmttrns = dom_get_tag("STMTTRN", tag_stmttrnrss[i]);
						tag_tbody = dom_create_tag("tbody", { "id": "acct" + i.toString() });
						
						if(tag_stmttrns.length > 0) {
							for(j = 0; j < tag_stmttrns.length; j++) {
								dtposted = dom_get_tag("DTPOSTED", tag_stmttrns[j])[0].firstChild.nodeValue;
								name = dom_get_tag("NAME", tag_stmttrns[j])[0].firstChild.nodeValue;
								trnamt = dom_get_tag("TRNAMT", tag_stmttrns[j])[0].firstChild.nodeValue;
								
								tag_tr = dom_create_tag("tr");
								
								tag_td = dom_create_tag("td", { "class": "dt" });
								tag_td.appendChild(dom_create_text(parseInt(dtposted.substring(4, 6), 10).toString() + "/" + parseInt(dtposted.substring(6, 8), 10).toString()));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "note" });
								tag_td.appendChild(dom_create_text(utf8_zentohan(name)));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "amt" });
								tag_td.appendChild(dom_create_text(to_amount(trnamt)));
								tag_tr.appendChild(tag_td);
								
								tag_tbody.appendChild(tag_tr);
							}
						} else {
							tag_tr = dom_create_tag("tr");
							
							tag_td = dom_create_tag("td", { "colspan": "3", "class": "dt" } );
							tag_td.appendChild(dom_create_text("明細がありません。"));
							tag_tr.appendChild(tag_td);
							
							tag_tbody.appendChild(tag_tr);
						}
						
						tag_table.appendChild(tag_tbody);
						
						
						// 売買一覧を生成する
						tag_invtrans = dom_get_tag("INVTRAN", tag_stmttrnrss[i]);
						tag_secids = dom_get_tag("SECID", tag_stmttrnrss[i]);
						tag_totals = dom_get_tag("TOTAL", tag_stmttrnrss[i]);
						
						if(tag_invtrans.length > 0) {
							tag_tbody = dom_create_tag("tbody", { "id": "acct" + tag_stmttrnrss.length.toString() });
							
							tag_option = dom_create_tag("option", { "value": tag_stmttrnrss.length.toString() });
							tag_option.appendChild(dom_create_text("売買"));
							tag_select.appendChild(tag_option);
							
							for(j = 0; j < tag_invtrans.length; j++) {
								dttrade = dom_get_tag("DTTRADE", tag_invtrans[j])[0].firstChild.nodeValue;
								security = securitys[dom_get_tag("UNIQUEIDTYPE", tag_secids[j])[0].firstChild.nodeValue + " " + dom_get_tag("UNIQUEID", tag_secids[j])[0].firstChild.nodeValue];
								total = tag_totals[j].firstChild.nodeValue;
								
								tag_tr = dom_create_tag("tr");
								
								tag_td = dom_create_tag("td", { "class": "dt" });
								tag_td.appendChild(dom_create_text(parseInt(dttrade.substring(4, 6), 10).toString() + "/" + parseInt(dttrade.substring(6, 8), 10).toString()));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "note" });
								tag_td.appendChild(dom_create_text(utf8_zentohan((parseInt(total, 10) > 0? "売付": "買付") + " " + security)));
								tag_tr.appendChild(tag_td);
								
								tag_td = dom_create_tag("td", { "class": "amt" });
								tag_td.appendChild(dom_create_text(to_amount(Math.abs(total))));
								tag_tr.appendChild(tag_td);
								
								tag_tbody.appendChild(tag_tr);
							}
							
							tag_tbody.appendChild(tag_tr);
							tag_table.appendChild(tag_tbody);
						}
						
						// 有価証券残高一覧を生成する
						tag_invposlists = dom_get_tag("INVPOSLIST", tag_stmttrnrss[i]);
						
						if(tag_invposlists.length > 0) {
							uniqueidtypes = dom_get_tag("UNIQUEIDTYPE", tag_invposlists[0]);
							uniqueids = dom_get_tag("UNIQUEID", tag_invposlists[0]);
							dtpriceasofs = dom_get_tag("DTPRICEASOF", tag_invposlists[0]);
							mktvals = dom_get_tag("MKTVAL", tag_invposlists[0]);
							
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
									
									tag_td = dom_create_tag("td", { "class": "dt" });
									tag_td.appendChild(dom_create_text(parseInt(dtpriceasof.substring(4, 6), 10).toString() + "/" + parseInt(dtpriceasof.substring(6, 8), 10).toString()));
									tag_tr.appendChild(tag_td);
									
									tag_td = dom_create_tag("td", { "class": "note" });
									tag_td.appendChild(dom_create_text(utf8_zentohan(name)));
									tag_tr.appendChild(tag_td);
									
									tag_td = dom_create_tag("td", { "class": "amt" });
									tag_td.appendChild(dom_create_text(to_amount(mktval)));
									tag_tr.appendChild(tag_td);
									
									tag_tbody.appendChild(tag_tr);
								}
								
								tag_tbody.appendChild(tag_tr);
								tag_table.appendChild(tag_tbody);
							}
						}
						
						tag_div.appendChild(tag_table);
						cdf.appendChild(tag_div);
					}
					
					// モーダルウィンドウを開く
					modal_show("明細", cdf, false, "acct");
					
					fnc_detail_change();
				}
			}
		} else {
			// モーダルウィンドウを閉じる
			modal_hide();
		}
	}
	
	return;
}

// 選択中以外の明細を非表示にする
function fnc_detail_change() {
	var acct = dom_get_id("acct").value;
	var obj;
	var i = 0;
	
	while(true) {
		obj = dom_get_id("acct" + i.toString());
		if(obj == null) break;
		obj.style.display = (i == acct)? "table-row-group": "none";
		i++;
	}
	
	return;
}

// OFX結合ダウンロード機能
function fnc_ofx_all() {
	var parser = null;
	var serializer = null;
	var merge = null;
	var current = null;
	var f = false;
	var tag_section = dom_get_tag("section")[0];
	var logons, auths, timestamp, settings, str;
	var blob;
	var tag_ofx, tag_signonmsgsrsv1, tag_sonrs, tag_status, tag_code, tag_severity, tag_dtserver, tag_language, tag_fi, tag_org, tag_bankmsgsrsv1, tag_creditcardmsgsrsv1, tag_invstmtmsgsrsv1, tag_seclistmsgsrsv1, tag_stmttrnrss, tag_seclist, tag_ccstmttrnrss, tag_invstmttrnrss, tag_seclists, tag_seclisttrnrs, tag_trnuid;
	var tag_a;
	var i, j, k;
	
	if(chkenv_ofx_all() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、OFXの結合ダウンロードに対応していません。", false);
	} else {
		parser = new DOMParser();
		serializer = new XMLSerializer();
		
		merge = parser.parseFromString("<OFX></OFX>", "text/xml");
		logons = local_current();
		auths = storage_get(logons["localid"], logons["localpass"]).split("\r\n");
		timestamp = timestamp_get();
		
		// 結合先を初期化する
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
		tag_org.appendChild(merge.createTextNode("<!--[family]-->/" + ver));
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
		
		// 結合先にデータを結合する
		for(i = 0; i < auths.length; i++) {
			settings = auth_parse(auths[i]);
			
			str = storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			if(str != null && str != "") {
				try {
					current = parser.parseFromString(str, "text/xml");
				} catch(e) {
					void(e);
				}
				
				// 銀行・前払式帳票
				tag_stmttrnrss = (current != null? dom_get_tag("STMTTRNRS", current): new Array());
				for(j = 0; j < tag_stmttrnrss.length; j++) {
					tag_bankmsgsrsv1.appendChild(tag_stmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// クレジットカード
				tag_ccstmttrnrss = (current != null? dom_get_tag("CCSTMTTRNRS", current): new Array());
				for(j = 0; j < tag_ccstmttrnrss.length; j++) {
					tag_creditcardmsgsrsv1.appendChild(tag_ccstmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// 証券
				tag_invstmttrnrss = (current != null? dom_get_tag("INVSTMTTRNRS", current): new Array());
				for(j = 0; j < tag_invstmttrnrss.length; j++) {
					tag_invstmtmsgsrsv1.appendChild(tag_invstmttrnrss[j].cloneNode(true));
					f = true;
				}
				
				// 証券
				tag_seclists = (current != null? dom_get_tag("SECLIST", current): new Array());
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
		
		// データをダウンロードする
		if(f == false) {
			modal_showonly("警告", "ダウンロード可能なOFXがありません。", false);
		} else {
			// データをダウンロードする
			blob = new Blob([str]);
			filedownload(blob, fprefix + timestamp + ".ofx", "application/x-ofx");
			blob = null;
		}
	}
	
	return;
}

// OFXダウンロード機能
function fnc_ofx(rowid) {
	var auth = auth_get(rowid);
	var logons = local_current();
	var settings = auth_parse(auth);
	var tag_section = dom_get_tag("section")[0];
	var blob;
	var tag_a;
	
	if(chkenv_export() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、OFXファイルのダウンロードに対応していません。", false);
	} else {
		// データをダウンロードする
		blob = new Blob([storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"])]);
		filedownload(blob, settings["fiid"] + (settings["keyvalues"]["timestamp"] != ""? "_" + settings["keyvalues"]["timestamp"]: "") + ".ofx", "application/x-ofx");
		blob = null;
	}
	
	return;
}

// CSVダウンロード機能
function fnc_csv() {
	var parser = null;
	var current = null;
	var f = false;
	var tag_section = dom_get_tag("section")[0];
	var title = dom_get_tag("title")[0].firstChild.nodeValue;
	var logons = local_current();
	var auths, timestamp, settings, str, total;
	var blob;
	var tag_stmttrnrss, tag_ccstmttrnrss, tag_invstmttrnrss, tag_stmttrns, tag_secinfos, tag_invposs;
	var mktginfo, balamt, availcash, dtasof, bankid, branchid, brokerid, acctid, dtposted, name, trnamt, memo, secname, uniqueid, dtpriceasof, mktval;
	var buf, buf_sjis, buf_blob, buf_view;
	var tag_a;
	var csvencoding;
	var i, j, k;
	
	if(chkenv_csv() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、CSVファイルのダウンロードに対応していません。", false);
	} else {
		parser = new DOMParser();
		
		auths = storage_get(logons["localid"], logons["localpass"]).split("\r\n");
		timestamp = timestamp_get();
		total = dom_get_id("num_total").firstChild.nodeValue.replace(/,/g, "");
		
		// CSVファイルの文字エンコーディングを取得する
		csvencoding = storage_get(logons["localid"] + ":csvencoding", logons["localpass"]);
		if(csvencoding == null) for(i in csvencodings) {
			csvencoding = i;
			break;
		}
		
		// データを生成する
		buf = "\"金融機関\",\"日付\",\"摘要\",\"金額\",\"メモ\"\r\n";
		buf += "\"" + title + " Version " + ver + "\"," + timestamp.substring(0, 4) + "-" + timestamp.substring(4, 6) + "-" + timestamp.substring(6, 8) + "," + "\"残高合計\"," + total + ",\"" + logons["localid"] + "\"\r\n";
		buf += "\r\n";
		
		for(i = 0; i < auths.length; i++) {
			settings = auth_parse(auths[i]);
			
			str = storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
			if(str != null && str != "") {
				try {
					current = parser.parseFromString(str, "text/xml");
				} catch(e) {
					void(e);
				}
				
				// 銀行・前払式帳票
				tag_stmttrnrss = (current != null? dom_get_tag("STMTTRNRS", current): new Array());
				for(j = 0; j < tag_stmttrnrss.length; j++) {
					mktginfo = dom_get_tag("MKTGINFO", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					balamt = dom_get_tag("BALAMT", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					dtasof = dom_get_tag("DTASOF", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					bankid = dom_get_tag("BANKID", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					branchid = dom_get_tag("BRANCHID", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					acctid = dom_get_tag("ACCTID", tag_stmttrnrss[j])[0].firstChild.nodeValue;
					tag_stmttrns = dom_get_tag("STMTTRN", tag_stmttrnrss[j]);
					
					buf += "\"" + mktginfo + "\"," + dtasof.substring(0, 4) + "-" + dtasof.substring(4, 6) + "-" + dtasof.substring(6, 8) + ",\"残高\"," + balamt + ",\"" + bankid + " " + branchid + " " + acctid + "\"\r\n";
					
					for(k = 0; k < tag_stmttrns.length; k++) {
						dtposted = dom_get_tag("DTPOSTED", tag_stmttrns[k])[0].firstChild.nodeValue;
						name = dom_get_tag("NAME", tag_stmttrns[k])[0].firstChild.nodeValue;
						trnamt = dom_get_tag("TRNAMT", tag_stmttrns[k])[0].firstChild.nodeValue;
						try {
							memo = dom_get_tag("MEMO", tag_stmttrns[k])[0].firstChild.nodeValue;
						} catch(e) {
							memo = "";
						}
						
						buf += "," + dtposted.substring(0, 4) + "-" + dtposted.substring(4, 6) + "-" + dtposted.substring(6, 8) + ",\"" + name + "\"," + trnamt + ",\"" + memo + "\"\r\n";
					}
					
					if(tag_stmttrnrss.length > 1 && j < tag_stmttrnrss.length - 1) buf += "\r\n";
					f = true;
				}
				
				// クレジットカード
				tag_ccstmttrnrss = (current != null? dom_get_tag("CCSTMTTRNRS", current): new Array());
				for(j = 0; j < tag_ccstmttrnrss.length; j++) {
					mktginfo = dom_get_tag("MKTGINFO", tag_ccstmttrnrss[j])[0].firstChild.nodeValue;
					balamt = dom_get_tag("BALAMT", tag_ccstmttrnrss[j])[0].firstChild.nodeValue;
					dtasof = dom_get_tag("DTASOF", tag_ccstmttrnrss[j])[0].firstChild.nodeValue;
					acctid = dom_get_tag("ACCTID", tag_ccstmttrnrss[j])[0].firstChild.nodeValue;
					tag_stmttrns = dom_get_tag("STMTTRN", tag_ccstmttrnrss[j]);
					
					buf += "\"" + mktginfo + "\"," + dtasof.substring(0, 4) + "-" + dtasof.substring(4, 6) + "-" + dtasof.substring(6, 8) + ",\"残高\"," + balamt + ",\"" + acctid + "\"\r\n";
					
					for(k = 0; k < tag_stmttrns.length; k++) {
						dtposted = dom_get_tag("DTPOSTED", tag_stmttrns[k])[0].firstChild.nodeValue;
						name = dom_get_tag("NAME", tag_stmttrns[k])[0].firstChild.nodeValue;
						trnamt = dom_get_tag("TRNAMT", tag_stmttrns[k])[0].firstChild.nodeValue;
						try {
							memo = dom_get_tag("MEMO", tag_stmttrns[k])[0].firstChild.nodeValue;
						} catch(e) {
							memo = "";
						}
						
						buf += "," + dtposted.substring(0, 4) + "-" + dtposted.substring(4, 6) + "-" + dtposted.substring(6, 8) + ",\"" + name + "\"," + trnamt + ",\"" + memo + "\"\r\n";
					}
					
					if(tag_ccstmttrnrss.length > 1 && j < tag_ccstmttrnrss.length - 1) buf += "\r\n";
					f = true;
				}
				
				// 証券
				tag_invstmttrnrss = (current != null? dom_get_tag("INVSTMTTRNRS", current): new Array());
				for(j = 0; j < tag_invstmttrnrss.length; j++) {
					mktginfo = dom_get_tag("MKTGINFO", tag_invstmttrnrss[j])[0].firstChild.nodeValue;
					availcash = dom_get_tag("AVAILCASH", tag_invstmttrnrss[j])[0].firstChild.nodeValue;
					dtasof = dom_get_tag("DTASOF", tag_invstmttrnrss[j])[0].firstChild.nodeValue;
					brokerid = dom_get_tag("BROKERID", tag_invstmttrnrss[j])[0].firstChild.nodeValue;
					acctid = dom_get_tag("ACCTID", tag_invstmttrnrss[j])[0].firstChild.nodeValue;
					tag_stmttrns = dom_get_tag("STMTTRN", tag_invstmttrnrss[j]);
					
					buf += "\"" + mktginfo + "\"," + dtasof.substring(0, 4) + "-" + dtasof.substring(4, 6) + "-" + dtasof.substring(6, 8) + ",\"残高\"," + availcash + ",\"" + brokerid + " " + acctid + "\"\r\n";
					
					for(k = 0; k < tag_stmttrns.length; k++) {
						dtposted = dom_get_tag("DTPOSTED", tag_stmttrns[k])[0].firstChild.nodeValue;
						name = dom_get_tag("NAME", tag_stmttrns[k])[0].firstChild.nodeValue;
						trnamt = dom_get_tag("TRNAMT", tag_stmttrns[k])[0].firstChild.nodeValue;
						try {
							memo = dom_get_tag("MEMO", tag_stmttrns[k])[0].firstChild.nodeValue;
						} catch(e) {
							memo = "";
						}
						
						buf += "," + dtposted.substring(0, 4) + "-" + dtposted.substring(4, 6) + "-" + dtposted.substring(6, 8) + ",\"" + name + "\"," + trnamt + ",\"" + memo + "\"\r\n";
					}
					
					if(tag_invstmttrnrss.length > 1 && j < tag_invstmttrnrss.length - 1) buf += "\r\n";
					f = true;
				}
				
				// 証券
				seclists = new Array();
				tag_secinfos = (current != null? dom_get_tag("SECINFO", current): new Array());
				for(j = 0; j < tag_secinfos.length; j++) {
					secname = dom_get_tag("SECNAME", tag_secinfos[j])[0].firstChild.nodeValue;
					uniqueid = dom_get_tag("UNIQUEID", tag_secinfos[j])[0].firstChild.nodeValue;
					seclists[uniqueid] = secname;
				}
				
				tag_invposs = (current != null? dom_get_tag("INVPOS", current): new Array());
				for(j = 0; j < tag_invposs.length; j++) {
					uniqueid = dom_get_tag("UNIQUEID", tag_invposs[j])[0].firstChild.nodeValue;
					dtpriceasof = dom_get_tag("DTPRICEASOF", tag_invposs[j])[0].firstChild.nodeValue;
					mktval = dom_get_tag("MKTVAL", tag_invposs[j])[0].firstChild.nodeValue;
					
					buf += "," + dtpriceasof.substring(0, 4) + "-" + dtpriceasof.substring(4, 6) + "-" + dtpriceasof.substring(6, 8) + ",\"" + seclists[uniqueid] + "\"," + mktval + ",\"" + uniqueid + "\"\r\n";
					f = true;
				}
				
				buf += "\r\n";
			}
		}
		
		// データをダウンロードする
		if(f == false) {
			modal_showonly("警告", "ダウンロード可能なCSVがありません。", false);
		} else {
			// ダウンロード用データを生成する
			switch(csvencoding) {
			case "UTFB":
				// BOMを追加する
				blob = new Blob([new Uint8Array([0xEF, 0xBB, 0xBF]), buf]);
				break;
			case "UTF8":
				blob = new Blob([buf]);
				break;
			case "SJIS":
			default:
				// CSVファイルの文字エンコーディングをShift_JISへと変換する
				buf_sjis = Encoding.convert(Encoding.stringToCode(buf), "SJIS", "UNICODE");
				j = buf_sjis.length;
				buf_blob = new ArrayBuffer(j);
				buf_view = new Uint8Array(buf_blob);
				for(i = 0; i < j; i++) buf_view[i] = buf_sjis[i];
				blob = new Blob([buf_blob]);
				break;
			}
			
			// データをダウンロードする
			filedownload(blob, fprefix + timestamp + ".csv", "text/csv");
			blob = null;
		}
	}
	
	return;
}

// PDFダウンロード機能
function fnc_pdf() {
	var logons = local_current();
	var bufs = new Array();
	var tag_thead = dom_get_tag("thead")[0];
	var tag_tfoot = dom_get_tag("tfoot")[0];
	var tag_section = dom_get_tag("section")[0];
	var tag_tbodys = dom_get_tag("tbody");
	var tag_tr;
	var title = dom_get_tag("title")[0].firstChild.nodeValue;
	var timestamp = timestamp_get();
	var pdf = pdftext;
	var pagestream = " ";
	var pdflength = "<<\r\n/Length <!--[length]-->\r\n>>\r\n";
	var pdfstream = "stream\r\n<!--[stream]-->endstream\r\n";
	var ua = navigator.userAgent;
	var content = "";
	var reference = "";
	var objstart = 1;  // obj開始番号
	var pagecount = 1; // ページ開始番号
	var phl = 772; // ページ先頭の高さ
	var pfl = 68; // ページ末尾の高さ
	var thl = 58; // ヘッダーの高さ
	var tfl = 28; // フッターの高さ
	var cpt = 10.5; // 文字サイズ（単位はpt）
	var cwl = cpt / 2; // 文字幅
	var chl = cpt + 1.5; // 文字高
	var fil = 11; // 金融機関の折り返し文字数
	var ail = 20; // 口座種目の折り返し文字数
	var mbl = 11; // 残高の折り返し文字数
	var udl = 11; // 更新日時の折り返し文字数
	var ldl; // PDFファイルの行ピッチ
	var tocobj, pdfobj, pdfchar, pdfdraw, pdfrowpitch;
	var buf;
	var row1, row2, row3;
	var i, j, k, l;
	var y;
	var objnum;
	var blob;
	
	if(chkenv_pdf() == false) {
		modal_showonly("警告", "ご利用のブラウザーは、PDFファイルのダウンロードに対応していません。", false);
	} else {
		// PDFファイルの行ピッチを取得する
		pdfrowpitch = storage_get(logons["localid"] + ":pdfrowpitch", logons["localpass"]);
		if(pdfrowpitch == null) for(i in pdfrowpitchs) if(pdfrowpitchs[i] == pdfdefsetting) {
			pdfrowpitch = i;
			break;
		}
		ldl = parseInt(pdfrowpitch, 10);
		
		ua = (ua.match(/[^\x20-\x7E]/)? "<!--[family]-->": ua.replace(/\(/g, "\\(").replace(/\)/g, "\\)"));
		
		// 埋め込み文字列を置換する
		pdf = pdf.replace("<!--[title]-->", "<!--[family]-->");
		pdf = pdf.replace("<!--[subject]-->", logons["localid"]);
		pdf = pdf.replace("<!--[creator]-->", "<!--[family]-->/" + ver);
		pdf = pdf.replace("<!--[producer]-->", ua);
		pdf = pdf.replace("<!--[creationdate]-->", timestamp);
		
		// obj生成開始番号を取得する
		objnum = objstart;
		while(pdf.indexOf(objnum.toString() + " 0 obj") != -1) objnum++;
		
		// 未出力明細が存在する場合、ループする
		i = 0;
		do {
			// 新規ページを生成する
			y = phl;
			pagestream += objnum.toString() + " 0 R ";
			tocobj = fnc_pdf_tocobj(objnum++);
			pdfobj = fnc_pdf_pdfobj(objnum++);
			pdfchar = fnc_pdf_charhead(title, logons["localid"], cpt, cwl, udl, pagecount, y);
			pdfdraw = fnc_pdf_drawhead();
			y -= thl;
			
			// 未出力明細が存在する場合
			while(i < tag_tbodys.length) {
				// ボディーを生成する
				tag_tr = dom_get_tag("tr", tag_tbodys[i]);
				
				// 明細1件あたりの出力行数を計算する
				row1 = Math.max(tag_tr.length, utf8_split(tag_tr[0].childNodes[0].firstChild.firstChild.nodeValue, fil).length);
				row2 = 0;
				for(j = tag_tr.length - 1; j >= 0; j--) row2 += utf8_split(tag_tr[tag_tr.length - j - 1].childNodes[(j == tag_tr.length - 1? 1: 0)].firstChild.nodeValue, ail).length;
				row1 = Math.max(row1, row2);
				l = row1 * pdfrowpitch;
				
				// 改行する
				y -= l;
				
				// ページ末尾に収まらない場合、ループを抜ける
				if(y < pfl) break;
				
				// 偶数行を着色する
				buf = l.toString();
				if(i % 2 == 0) pdfdraw += "q\r\n1 0 0 1 57 " + y.toString() + " cm\r\n0.8 0.933 1 rg\r\n0 0 m\r\n480 0 l\r\n480 " + buf + " l\r\n0 " + buf + " l\r\nf\r\nQ\r\n";
				
				// 金融機関を出力する
				bufs = utf8_split(tag_tr[0].childNodes[0].firstChild.firstChild.nodeValue, fil);
				for(j = row1 - 1; j >= 0; j--) if(bufs[row1 - j - 1]) pdfchar += "1 0 0 1 60 " + (y + 2 + pdfrowpitch * j + (pdfrowpitch - chl) / 2).toString() + " Tm\r\n<" + get_binary_sjis(bufs[row1 - j - 1]) + "> Tj\r\n";
				
				// 口座種目の行数を計算する
				row2 = 0;
				for(j = tag_tr.length - 1; j >= 0; j--) row2 += utf8_split(tag_tr[tag_tr.length - j - 1].childNodes[(j == tag_tr.length - 1? 1: 0)].firstChild.nodeValue, ail).length - 1;
				row2 = Math.max(row1, row2);
				
				// 口座種目を出力する
				l = row2;
				for(j = tag_tr.length - 1; j >= 0; j--) {
					bufs = utf8_split(tag_tr[tag_tr.length - j - 1].childNodes[(j == tag_tr.length - 1? 1: 0)].firstChild.nodeValue, ail);
					row3 = bufs.length;
					for(k = row3 - 1; k >= 0; k--) {
						pdfchar += "1 0 0 1 186.5 " + (y + 2 + pdfrowpitch * (l - 1) + (pdfrowpitch - chl) / 2).toString() + " Tm\r\n<" + get_binary_sjis(bufs[row3 - k - 1]) + "> Tj\r\n";
						l--;
					}
				}
				
				if(tag_tr[0].childNodes[1].colSpan == 1) {
					// 残高を出力する
					l = row2;
					for(j = tag_tr.length - 1; j >= 0; j--) {
						buf = tag_tr[tag_tr.length - j - 1].childNodes[(j == tag_tr.length - 1? 2: 1)].firstChild.nodeValue;
						pdfchar += "1 0 0 1 " + (407 + cwl * (mbl - buf.length)).toString() + " " + (y + 2 + pdfrowpitch * (l - 1) + (pdfrowpitch - chl) / 2).toString() + " Tm\r\n<" + get_binary_sjis(buf) + "> Tj\r\n";
						l--;
					}
					
					// 更新日時を出力する
					buf = tag_tr[0].childNodes[3].firstChild.nodeValue;
					pdfchar += "1 0 0 1 " + (476 + cwl * (udl - buf.length)).toString() + " " + (y + 2 + pdfrowpitch * (row1 - 1) + (pdfrowpitch - chl) / 2).toString() + " Tm\r\n<" + get_binary_sjis(buf) + "> Tj\r\n";
				}
				
				i++;
			}
			
			// ページ末尾の場合
			if(y < pfl) {
				// 改ページする
				pdfchar += fnc_pdf_charfoot(mbl, cwl, udl, y, false);
				pdfdraw += fnc_pdf_drawfoot(y, false);
				
				// 埋め込み文字列を置換する
				pdfobj = pdfobj.replace("<!--[pdfobj]-->", pdflength.replace("<!--[length]-->", (pdfdraw.length + pdfchar.length).toString()) + pdfstream.replace("<!--[stream]-->", pdfdraw + pdfchar));
				content += tocobj + pdfobj;
				pagecount++;
			}
		} while(i < tag_tbodys.length);
		
		// フッターを出力する高さがない場合
		if(y - tfl < pfl) {
			// 改ページする
			pdfchar += fnc_pdf_charfoot(mbl, cwl, udl, y, false);
			pdfdraw += fnc_pdf_drawfoot(y, false);
			
			// 埋め込み文字列を置換する
			pdfobj = pdfobj.replace("<!--[pdfobj]-->", pdflength.replace("<!--[length]-->", (pdfdraw.length + pdfchar.length).toString()) + pdfstream.replace("<!--[stream]-->", pdfdraw + pdfchar));
			content += tocobj + pdfobj;
			pagecount++;
			
			// 新規ページを生成する
			y = phl;
			pagestream += objnum.toString() + " 0 R ";
			tocobj = fnc_pdf_tocobj(objnum++);
			pdfobj = fnc_pdf_pdfobj(objnum++);
			pdfchar = fnc_pdf_charhead(title, logons["localid"], cpt, cwl, udl, pagecount, y);
			pdfdraw = fnc_pdf_drawhead();
			y -= thl;
		}
		
		// フッターを生成する
		pdfchar += fnc_pdf_charfoot(mbl, cwl, udl, y, true);
		pdfdraw += fnc_pdf_drawfoot(y, true);
		
		// 埋め込み文字列を置換する
		pdfobj = pdfobj.replace("<!--[pdfobj]-->", pdflength.replace("<!--[length]-->", (pdfdraw.length + pdfchar.length).toString()) + pdfstream.replace("<!--[stream]-->", pdfdraw + pdfchar));
		content += tocobj + pdfobj;
		
		pdf = pdf.replace("<!--[pagestream]-->", pagestream);
		pdf = pdf.replace("<!--[pagecount]-->", pagecount.toString());
		pdf = pdf.replace("<!--[content]-->", content);
		pdf = pdf.replace("<!--[objnum]-->", objnum.toString());
		
		// objの開始位置より相互参照を生成する
		for(i = objstart; i < objnum; i++) {
			buf = pdf.indexOf(i.toString() + " 0 obj").toString();
			while(buf.length < 10) buf = "0" + buf;
			reference += buf + " 00000 n\r\n";
		}
		
		pdf = pdf.replace("<!--[reference]-->", reference);
		pdf = pdf.replace("<!--[xref]-->", pdf.indexOf("xref").toString());
		
		// データをダウンロードする
		blob = new Blob([pdf]);
		filedownload(blob, fprefix + timestamp + ".pdf", "application/pdf");
		blob = null;
	}
	
	return;
}

// tocobjを生成する
function fnc_pdf_tocobj(objnum) {
	var ret = "";
	
	ret += "\r\n";
	ret += objnum.toString() + " 0 obj\r\n";
	ret += "<<\r\n";
	ret += "/Type /Page\r\n";
	ret += "/Parent 7 0 R\r\n";
	ret += "/Resources 1 0 R\r\n";
	ret += "/Contents " + (objnum + 1).toString() + " 0 R\r\n";
	ret += ">>\r\n";
	ret += "endobj\r\n";
	
	return ret;
}

// pdfobjを生成する
function fnc_pdf_pdfobj(objnum) {
	var ret = "";
	
	ret += "\r\n";
	ret += objnum.toString() + " 0 obj\r\n";
	ret += "<!--[pdfobj]-->endobj\r\n";
	
	return ret;
}

// pdfcharのヘッダーを生成する
function fnc_pdf_charhead(title, localid, cpt, cwl, udl, pagecount, y) {
	var ret = "";
	var tag_thead = dom_get_tag("thead")[0];
	var tag_tr = dom_get_tag("tr", tag_thead)[0];
	var buf = "Page " + pagecount.toString();
	
	ret += "BT\r\n";
	
	ret += "/F1 18 Tf\r\n";
	ret += "1 0 0 1 60 " + y.toString() + " Tm\r\n<" + get_binary_sjis(title) + "> Tj\r\n"; // マネーサウンド
	ret += "/F1 10.5 Tf\r\n";
	ret += "1 0 0 1 60 " + (y - 18).toString() + " Tm\r\n<" + get_binary_sjis(localid) + "> Tj\r\n"; // ローカルID
	ret += "1 0 0 1 " + (476 + cwl * (udl - buf.length)).toString() + " " + (y - 18).toString() + " Tm\r\n<" + get_binary_sjis(buf) + "> Tj\r\n"; // ページ番号
	
	ret += "/F1 " + cpt.toString() + " Tf\r\n";
	ret += "1 0 0 1 97.25 " + (y - 47).toString() + " Tm\r\n<" + get_binary_sjis(tag_tr.childNodes[0].firstChild.nodeValue) + "> Tj\r\n"; // 金融機関
	ret += "1 0 0 1 270 " + (y - 47).toString() + " Tm\r\n<" + get_binary_sjis(tag_tr.childNodes[1].firstChild.nodeValue) + "> Tj\r\n"; // 口座種目
	ret += "1 0 0 1 425.25 " + (y - 47).toString() + " Tm\r\n<" + get_binary_sjis(tag_tr.childNodes[2].firstChild.nodeValue) + "> Tj\r\n"; // 残高
	ret += "1 0 0 1 484 " + (y - 47).toString() + " Tm\r\n<" + get_binary_sjis(tag_tr.childNodes[3].firstChild.nodeValue) + "> Tj\r\n"; // 更新日時
	
	return ret;
}

// pdfcharのフッターを生成する
function fnc_pdf_charfoot(mbl, cwl, udl, y, last) {
	var ret = "";
	var tag_tfoot = dom_get_tag("tfoot")[0];
	var tag_tr = dom_get_tag("tr", tag_tfoot)[0];
	var buf = tag_tr.childNodes[2].firstChild.nodeValue;
	
	if(last == true) {
		ret += "1 0 0 1 186.5 " + (y - 18).toString() + " Tm\r\n<" + get_binary_sjis(tag_tr.childNodes[1].firstChild.nodeValue) + "> Tj\r\n"; // 合計
		ret += "1 0 0 1 " + (407 + cwl * (mbl - buf.length)).toString() + " " + (y - 18).toString() + " Tm\r\n<" + get_binary_sjis(buf) + "> Tj\r\n"; // 残高
	}
	
	ret += "ET\r\n";
	
	return ret;
}

// pdfdrawのヘッダーを生成する
function fnc_pdf_drawhead() {
	var ret = "";
	
	ret += "q\r\n1.5 w\r\n1 0 0 1 57 742 cm\r\n0 0 0 rg\r\n0 0 m\r\n480 0 l\r\n480 2.5 l\r\n0 2.5 l\r\nf\r\nQ\r\n";
	ret += "q\r\n0.5 w\r\n1 0 0 1 57 714.5 cm\r\n0 0 0 rg\r\n0 0 m\r\n480 0 l\r\nS\r\nQ\r\n";
	
	return ret;
}

// pdfdrawのフッターを生成する
function fnc_pdf_drawfoot(y, last) {
	var ret = "";
	
	if(last == true) {
		ret += "q\r\n0.5 w\r\n1 0 0 1 57 " + (y - 0.5).toString() + " cm\r\n0 0 0 rg\r\n0 0 m\r\n480 0 l\r\nS\r\nQ\r\n";
		ret += "q\r\n0.5 w\r\n1 0 0 1 57 " + (y - 28).toString() + " cm\r\n0 0 0 rg\r\n0 0 m\r\n480 0 l\r\nS\r\nQ\r\n";
	}
	
	return ret;
}

// 口座一覧印刷機能
function fnc_print() {
	// ブラウザーの印刷モーダルウィンドウを呼び出す
	self.window.print();
	
	return;
}

// 口座情報エクスポート機能
function fnc_export() {
	var logons = local_current();
	var auth = storage_get(logons["localid"], logons["localpass"]);
	var cdf = document.createDocumentFragment();
	var blob;
	var tag_section = dom_get_tag("section")[0];
	var tag_p, tag_input;
	var pass;
	var enc;
	
	if(dom_get_id("modal") == null) {
		tag_p = dom_create_tag("p", { "class": "label" });
		tag_p.appendChild(dom_create_text(fiids["logon"][fiids["logon"]["form"].split("|")[1]].split("|")[0] + "の変更（オプション）"));
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_input = dom_create_tag("input", { "type": "password", "name": "pass", "id": "pass", "value": logons["localpass"], "onkeyup": "form_empty_check();", "onblur": "this.onkeyup();" });
		tag_p.appendChild(tag_input);
		cdf.appendChild(tag_p);
		
		tag_p = dom_create_tag("p");
		tag_input = dom_create_tag("input", { "type": "hidden", "name": "auth", "id": "auth", "value": auth.replace(/\r\n/gm, "\n") });
		tag_p.appendChild(tag_input);
		cdf.appendChild(tag_p);
		
		// モーダルウィンドウを開く
		modal_show("口座情報のエクスポート", cdf, true, "pass");
	} else {
		// コールバックの場合
		pass = dom_get_id("pass").value;
		auth = dom_get_id("auth").value.replace(/\n/gm, "\r\n");
		
		// モーダルウィンドウを閉じる
		modal_hide();
		
		enc = "data:application/octet-stream;ver=" + ver + ";base64," + CryptoJS.AES.encrypt((pass + "\t" + auth.replace(/\t?(status|timestamp)\=[^\t\r\n]+/gm, "") + "\r\n").toString(CryptoJS.enc.Utf8), pass).toString();
		
		if(chkenv_export() == false) {
			tag_p = dom_create_tag("p");
			tag_p.appendChild(dom_create_text("以下のデータをコピー&amp;ペーストし、保存してください。"));
			cdf.appendChild(tag_p);
			
			tag_p = dom_create_tag("p", { "class": "label" });
			tag_p.appendChild(dom_create_text("口座情報"));
			cdf.appendChild(tag_p);
			
			tag_p = dom_create_tag("p");
			tag_input = dom_create_tag("input", { "type": "text", "name": "enc", "id": "enc", "value": enc, "readonly": "readonly", "onfocus": "this.select();" });
			tag_p.appendChild(tag_input);
			cdf.appendChild(tag_p);
			
			modal_showonly("口座情報のエクスポート", cdf, false, "enc");
		} else {
			// データをダウンロードする
			blob = new Blob([enc]);
			filedownload(blob, fprefix + logons["localid"] + ".txt", "text/plain");
			blob = null;
		}
	}
	
	return;
}

// 口座一覧表示機能
function fnc_list_all(lists) {
	var logons = local_current();
	var tag_table = dom_get_tag("table")[0];
	var tag_tbodys = dom_get_tag("tbody", tag_table);
	var f = false;
	var btn_disableds = { "btn_update_all": false, "btn_ofx_all": false, "btn_output": false };
	var i;
	
	// 一覧からすべての行を取り除く
	for(i = tag_tbodys.length - 1; i >= 0; i--) tag_table.removeChild(tag_tbodys[i]);
	
	// 行を追加する
	for(i = 0; i < lists.length; i++) {
		if(lists[i].length == 0) continue;
		tag_table.appendChild(fnc_list(lists[i]));
		f = true;
	}
	
	// 行を追加した場合、ボタンを有効・無効に設定する
	if(f == true) for(i in btn_disableds) dom_get_id(i).disabled = btn_disableds[i];
	
	// 口座一覧を更新する
	num_total_update();
	
	return;
}

// 口座表示機能
function fnc_list(list) {
	var logons = local_current();
	var settings = auth_parse(list);
	var str = storage_get(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
	var ofx = null;
	var parser = null;
	var broken = false;
	var inactive = false;
	var group = "";
	var caption = "";
	var status = "";
	var timestamp = "";
	var tag_tbody, tag_tr, tag_td, tag_a;
	var banks, creditcards, investments;
	var balamt, mktginfo, bankacctfrom, bankid, branchid, acctid, accttype, ccacctfrom, stmttrns, stmttrn, marginbalance, invacctfrom, brokerid, acctid, mktval, invposlist, mktvals;
	var ofxbutton, inputs, disabled;
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
	
	// 認証情報よりデータを抽出する
	for(i in settings["keyvalues"]) switch(i) {
	case "status":
		status = settings["keyvalues"][i];
		break;
	case "timestamp":
		with(settings["keyvalues"][i]) timestamp = parseInt(substring(4, 6), 10).toString() + "/" + parseInt(substring(6, 8), 10).toString() + " " + substring(8, 10) + ":" + substring(10, 12);
		break;
	default:
		break;
	}
	
	// 金融機関が未定義の場合、fiidを表示する
	if(typeof fiids[settings["fiid"]] == "undefined") {
		fiids[settings["fiid"]] = new Array();
		fiids[settings["fiid"]]["name"] = settings["fiid"];
		fiids[settings["fiid"]]["home"] = "about:blank";
		inactive = true;
	}
	
	// ヘルプが未定義の場合、固定文字列を表示する
	if(typeof fiids[settings["fiid"]]["help"] == "undefined") fiids[settings["fiid"]]["help"] = "ヘルプはありません。";
	
	// OFXよりデータを抽出する
	try {
		// 銀行・前払式帳票
		banks = (ofx != null? dom_get_tag("STMTTRNRS", ofx): new Array());
		for(i = 0; i < banks.length; i++) {
			balamt = parseInt(dom_get_tag("BALAMT", banks[i])[0].firstChild.nodeValue, 10);
			mktginfo = (dom_get_tag("MKTGINFO", banks[i]).length == 0? "": dom_get_tag("MKTGINFO", banks[i])[0].firstChild.nodeValue);
			bankacctfrom = dom_get_tag("BANKACCTFROM", banks[i])[0];
			
			bankid = dom_get_tag("BANKID", bankacctfrom)[0].firstChild.nodeValue;
			branchid = dom_get_tag("BRANCHID", bankacctfrom)[0].firstChild.nodeValue;
			acctid = dom_get_tag("ACCTID", bankacctfrom)[0].firstChild.nodeValue;
			accttype = dom_get_tag("ACCTTYPE", bankacctfrom)[0].firstChild.nodeValue;
			
			j = mktginfo.indexOf("　");
			group = (accttype == "SAVINGS"? (j == -1? "預金": mktginfo.substring(j + 1)) + " " + acctid.substring(3, 7): (j == -1? acctid: mktginfo.substring(j + 1)));
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": banks.length.toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": "javascript: void(0);", "title": "ヘルプ", "onclick": "fnc_help(\"" + settings["fiid"] + "\");" });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": bankid + " " + (branchid == "0"? "": branchid + " ") + acctid });
			tag_td.appendChild(dom_create_text(utf8_zentohan(group)));
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
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "onclick": "fnc_update(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
		}
		
		// クレジットカード
		creditcards = (ofx != null? dom_get_tag("CCSTMTTRNRS", ofx): new Array());
		for(i = 0; i < creditcards.length; i++) {
			balamt = parseInt(dom_get_tag("BALAMT", creditcards[i])[0].firstChild.nodeValue, 10);
			mktginfo = (dom_get_tag("MKTGINFO", creditcards[i]).length == 0? "": dom_get_tag("MKTGINFO", creditcards[i])[0].firstChild.nodeValue);
			ccacctfrom = dom_get_tag("CCACCTFROM", creditcards[i])[0];
			
			acctid = dom_get_tag("ACCTID", ccacctfrom)[0].firstChild.nodeValue;
			
			j = mktginfo.indexOf("　");
			group = (j == -1? "預金": mktginfo.substring(j + 1)) + " " + acctid.substring(acctid.length - 4, acctid.length);
			
			// 明細の最終行がクレジットカード支払請求、かつ支払日が未到来の場合、残高より該当金額を差し引く
			stmttrns = dom_get_tag("STMTTRN", creditcards[i]);
			stmttrn = stmttrns[stmttrns.length - 1];
			if(dom_get_tag("NAME", stmttrn)[0].firstChild.nodeValue == fiids[settings["fiid"]]["name"] && dom_get_tag("DTPOSTED", stmttrn)[0].firstChild.nodeValue.substring(0, 8) > timestamp_get().substring(0, 8)) balamt -= parseInt(dom_get_tag("TRNAMT", stmttrn)[0].firstChild.nodeValue, 10);
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": creditcards.length.toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": "javascript: void(0);", "title": "ヘルプ", "onclick": "fnc_help(\"" + settings["fiid"] + "\");" });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": acctid });
			tag_td.appendChild(dom_create_text(utf8_zentohan(group)));
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
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "onclick": "fnc_update(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
		}
		
		// 証券
		investments = (ofx != null? dom_get_tag("INVSTMTTRNRS", ofx): new Array());
		for(i = 0; i < investments.length; i++) {
			marginbalance = parseInt(dom_get_tag("MARGINBALANCE", investments[i])[0].firstChild.nodeValue, 10);
			invacctfrom = dom_get_tag("INVACCTFROM", investments[i])[0];
			mktginfo = (dom_get_tag("MKTGINFO", investments[i]).length == 0? "": dom_get_tag("MKTGINFO", investments[i])[0].firstChild.nodeValue);
			
			brokerid = dom_get_tag("BROKERID", invacctfrom)[0].firstChild.nodeValue;
			acctid = dom_get_tag("ACCTID", invacctfrom)[0].firstChild.nodeValue;
			
			j = mktginfo.indexOf("　");
			group = "預金" + (j == -1? "": "　" + mktginfo.substring(j + 1));
			
			mktval = 0;
			invposlist = dom_get_tag("INVPOSLIST", investments[i])[0];
			mktvals = dom_get_tag("MKTVAL", invposlist);
			for(j = 0; j < mktvals.length; j++) mktval += parseInt(mktvals[j].firstChild.nodeValue, 10);
			
			tag_tr = dom_create_tag("tr");
			
			// 金融機関名称
			if(i == 0) {
				tag_td = dom_create_tag("td", { "rowspan": (investments.length + 1).toString(), "class": "fi" });
				tag_a = dom_create_tag("a", { "href": "javascript: void(0);", "title": "ヘルプ", "onclick": "fnc_help(\"" + settings["fiid"] + "\");" });
				tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
				tag_td.appendChild(tag_a);
				tag_tr.appendChild(tag_td);
			}
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": brokerid + " " + acctid });
			tag_td.appendChild(dom_create_text(utf8_zentohan(group)));
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
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "onclick": "fnc_update(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "onclick": "fnc_detail(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");" }));
				tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");" }));
				tag_tr.appendChild(tag_td);
			}
			
			tag_tbody.appendChild(tag_tr);
			
			tag_tr = dom_create_tag("tr");
			
			group = "有価証券";
			
			// 口座種目
			tag_td = dom_create_tag("td", { "class": "accttype", "title": brokerid + " " + acctid + "-1" });
			tag_td.appendChild(dom_create_text(utf8_zentohan(group)));
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
	
	// OFX破損の場合
	if(broken == true) status = "500";
	
	// statusが200、かつデータなしの場合
	if(status == 200 && banks.length == 0 && creditcards.length == 0 && investments.length == 0) {
		status = "204";
		broken = true;
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
	case "204":
		group = "データなし: 金融機関のサイトで状況を確認してください";
		tag_tbody.className = "error";
		break;
	case "403":
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
	case "511":
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
	
	// OFX破損、またはデータなしの場合
	if(broken == true || (banks.length == 0 && creditcards.length == 0 && investments.length == 0)) {
		tag_tr = dom_create_tag("tr");
		
		// 金融機関名称
		tag_td = dom_create_tag("td", { "rowspan": "1", "class": "fi" });
		tag_a = dom_create_tag("a", { "href": "javascript: void(0);", "title": "ヘルプ", "onclick": "fnc_help(\"" + settings["fiid"] + "\");" });
		tag_a.appendChild(dom_create_text(fiids[settings["fiid"]]["name"]));
		tag_td.appendChild(tag_a);
		tag_tr.appendChild(tag_td);
		
		// 口座種目
		tag_td = dom_create_tag("td", { "colspan": (status == "200"? "1": (timestamp != ""? "2": "3")), "class": "accttype", "title": caption });
		tag_td.appendChild(dom_create_text(utf8_zentohan(group)));
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
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "更新", "onclick": "fnc_update(\"" + settings["rowid"] + "\");" }));
		disabled = (status != "200" && parser != null? "true": "false");
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "明細", "disabled": disabled, "onclick": "fnc_detail(\"" + settings["rowid"] + "\");" }));
		disabled = (status != "200" && parser != null && debug == false? "true": "false");
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "OFX", "disabled": disabled, "onclick": "fnc_ofx(\"" + settings["rowid"] + "\");" }));
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "変更", "onclick": "fnc_modify(\"" + settings["rowid"] + "\");" }));
		tag_td.appendChild(dom_create_tag("input", { "type": "button", "value": "削除", "onclick": "fnc_delete(\"" + settings["rowid"] + "\");" }));
		tag_tr.appendChild(tag_td);
		
		tag_tbody.appendChild(tag_tr);
	}
	
	// OFXボタンの表示を取得する
	ofxbutton = storage_get(logons["localid"] + ":ofxbutton", logons["localpass"]);
	if(ofxbutton == null) for(i in ofxbuttons) {
		ofxbutton = i;
		break;
	}
	
	// OFXボタンの表示を制御する
	inputs = dom_get_tag("input", tag_tbody);
	for(i = 0; i < inputs.length; i++) with(inputs[i]) if(value == "OFX") style.display = (ofxbutton == "T"? "inline": "none");
	
	return tag_tbody;
}

// ヘルプ機能
function fnc_help(fiid) {
	var cdf = document.createDocumentFragment();
	var tag_div = dom_create_tag("div", { "id": "help" });
	var tag_h2 = dom_create_tag("h2");
	var tag_a = dom_create_tag("a", { "href": fiids[fiid]["home"], "target": fiid });
	
	tag_a.appendChild(dom_create_text(fiids[fiid]["name"] + "のサイト"));
	tag_h2.appendChild(tag_a);
	tag_div.innerHTML = fiids[fiid]["help"];
	tag_div.insertBefore(tag_h2, tag_div.firstChild);
	cdf.appendChild(tag_div);
	
	// モーダルウィンドウを開く
	modal_showonly("ヘルプ", cdf, false);
	
	return;
}


// =========================================================================
// 処理
// =========================================================================

// モーダルウィンドウを開く
function modal_show(mhead, mbody, showcancel, focusto) {
	var tag_bgs = [dom_get_tag("header")[0], dom_get_tag("nav")[0], dom_get_tag("section")[0], dom_get_tag("footer")[0]];
	var tag_tabs = [dom_get_tag("a"), dom_get_tag("input")];
	var tag_body = dom_get_tag("body")[0];
	var tag_article = dom_create_tag("article");
	var tag_div, tag_h3, tag_form, tag_aside;
	var lists;
	var fnc;
	var i, j, z;
	
	if(dom_get_id("modal") == null) {
		// オーバーレイの背景のフォーカスを禁止する
		for(i in tag_tabs) for(j in tag_tabs[i]) tag_tabs[i][j].tabIndex = -1;
		
		// モーダルウィンドウを生成する
		tag_form = dom_create_tag("form", { "method": "post", "id": "modal", "onsubmit": "(" + arguments.callee.caller + ")(); return false;", "onreset": "modal_hide();" });
		
		tag_h3 = dom_create_tag("h3", { "id": "modalhead" });
		tag_h3.appendChild(dom_create_text(mhead));
		tag_form.appendChild(tag_h3);
		
		tag_div = dom_create_tag("div", { "id": "modalbody" });
		tag_div.appendChild((typeof mbody == "string"? dom_create_text(mbody): mbody));
		tag_form.appendChild(tag_div);
		
		tag_div = dom_create_tag("div", { "id": "modalfoot" });
		tag_div.appendChild(dom_create_tag("input", { "type": "submit", "value": "OK", "id": "modalok" }));
		if(showcancel == true) tag_div.appendChild(dom_create_tag("input", { "type": "reset", "value": "キャンセル", "id": "modalcancel" }));
		
		tag_form.appendChild(tag_div);
		tag_article.appendChild(tag_form);
		tag_body.appendChild(tag_article);
		
		// モーダルウィンドウのタイトル部分のドラッグ＆ドロップを許可する
		tag_h3.onmousedown = function(e) {
			var target;
			
			if(typeof e == "undefined") e = self.window.event;
			target = e.target || e.srcElement;
			
			pw = true;
			
			with(this.parentNode) {
				px = e.clientX - offsetLeft;
				py = e.clientY - offsetTop;
			}
			
			return;
		};
		
		with(self.document) {
			// モーダルウィンドウがドラッグされた場合、移動を許可する
			onmousemove = function(e) {
				if(typeof e == "undefined") e = self.window.event;
				
				if(pw == true) with(tag_h3.parentNode.style) {
					left = (e.clientX - px).toString() + "px";
					top = (e.clientY - py).toString() + "px";
				}
				
				return;
			};
			
			// モーダルウィンドウがドロップされた場合、移動を禁止する
			onmouseup = function() {
				if(pw == true) pw = false;
				
				return;
			};
		}
		
		// ウィンドウサイズが変更された場合、画面サイズの変更を制御する
		self.window.onresize = function() {
			modal_resize();
			
			return;
		};
		
		// 親ウィンドウのスクロールを禁止する
		z = self.window.pageYOffset;
		with(tag_body.style) {
			position = "fixed";
			top = (z * -1).toString() + "px";
		}
		
		// デフォルトのフォーカスが指定されている場合、設定する
		if(typeof focusto == "string") with(dom_get_id(focusto)) {
			focus();
			if(tagName == "input") select();
		} else {
			// iOSのSafariブラウザー対策
			fnc = function() {
				dom_get_id("modalok").focus();
			};
			self.window.setTimeout(fnc, 100);
		}
		
		// オーバーレイを生成する
		tag_aside = dom_create_tag("aside");
		tag_article.appendChild(tag_aside);
		
		// オーバーレイの背景を処理する
		for(i in tag_bgs) tag_bgs[i].className = "bg";
		
		// 画面サイズの変更を制御する
		modal_resize();
	}
	
	return;
}

// モーダルウィンドウを開く（呼び出し元機能に戻らない）
function modal_showonly(mhead, mbody, showcancel, focusto) {
	if(dom_get_id("modal") == null) {
		// モーダルウィンドウを開く
		modal_show(mhead, mbody, showcancel, focusto);
	} else {
		// コールバックの場合、モーダルウィンドウを閉じる
		modal_hide();
	}
	
	return;
}

// モーダルウィンドウ表示時の画面サイズの変更を制御する
function modal_resize() {
	var tag_aside = dom_get_tag("aside")[0];
	var modal = dom_get_id("modal");
	var x, y;
	
	with(self.document) {
		x = documentElement.clientWidth || body.clientWidth || body.scrollWidth;
		y = documentElement.clientHeight || body.clientHeight || body.scrollHeight;
	}
	
	// オーバーレイのサイズを変更する
	if(tag_aside != null) with(tag_aside.style) {
		width = x;
		height = y;
	}
	
	// モーダルウィンドウを中央に表示する
	if(modal != null) with(modal) {
		style.left = parseInt((x - clientWidth) / 2, 10).toString() + "px";
		style.top = parseInt((y - clientHeight) / 2, 10).toString() + "px";
	}
	
	return;
}

// モーダルウィンドウを閉じる
function modal_hide() {
	var tag_bgs = [dom_get_tag("header")[0], dom_get_tag("nav")[0], dom_get_tag("section")[0], dom_get_tag("footer")[0]];
	var tag_tabs = [dom_get_tag("a"), dom_get_tag("input")];
	var tag_body = dom_get_tag("body")[0];
	var tag_article = dom_get_tag("article")[0];
	var lists;
	var i, j;
	var z;
	
	if(dom_get_id("modal") != null) {
		with(tag_body) {
			// モーダルウィンドウ・オーバーレイを削除する
			removeChild(tag_article);
			
			// 親ウィンドウのスクロールを許可する
			z = parseInt(style.top.replace("px", ""), 10) * -1;
			style.position = "static";
			style.top = "auto";
			self.window.scrollTo(0, z);
		}
		
		// オーバーレイの背景のフォーカスを許可する
		for(i in tag_tabs) for(j in tag_tabs[i]) tag_tabs[i][j].tabIndex = 0;
		
		// オーバーレイの背景を処理する
		for(i in tag_bgs) tag_bgs[i].className = "";
		
		with(self.document) {
			// モーダルウィンドウがドラッグ・ドロップされた場合、何もしない
			onmousemove = null;
			onmouseup = null;
		}
		
		// ウィンドウサイズが変更された場合、何もしない
		self.window.onresize = null;
	}
	
	return;
}

// データを取得する
function storage_get(key, pass) {
	var enc, dec;
	
	with(self.window) if(typeof pass == "string") {
		// ローカルストレージの場合、データを復号する
		enc = localStorage.getItem(key);
		if(enc == null) {
			// 暗号化データが存在しない場合
			dec = enc;
		} else {
			// 暗号化データを復号する
			dec = CryptoJS.AES.decrypt(enc, pass);
			dec = (dec.toString().substring(0, (pass + "\t").length * 2) == CryptoJS.enc.Utf8.parse(pass + "\t")? dec.toString(CryptoJS.enc.Utf8): "");
			dec = (dec == ""? "": dec.substring((pass + "\t").length));
		}
	} else {
		// セッションストレージの場合、データをそのまま取得する
		dec = sessionStorage.getItem(key);
	}
	
	return dec;
}

// データを設定する
function storage_set(key, value, pass) {
	with(self.window) if(typeof pass == "string") {
		// ローカルストレージの場合、データを暗号化する
		localStorage.setItem(key, CryptoJS.AES.encrypt(((pass + "\t") + (value != ""? value: "\r\n")).toString(CryptoJS.enc.Utf8), pass));
	} else {
		// セッションストレージの場合、データをそのまま設定する
		sessionStorage.setItem(key, value);
	}
	
	return;
}

// データを削除する
function storage_delete(key, pass) {
	var i;
	
	if(typeof pass == "string") {
		// ローカルストレージの場合
		with(self.window) if(pass == "*") {
			// passが「*」の場合、すべてのデータを削除する
			for(i = localStorage.length - 1; i >= 0; i--) if(localStorage.key(i).indexOf(key) == 0) localStorage.removeItem(localStorage.key(i));
		} else {
			// それ以外の場合、該当するデータを削除する
			localStorage.removeItem(key);
		}
	} else {
		// セッションストレージの場合、データをそのまま削除する
		self.window.sessionStorage.removeItem(key);
	}
	
	return;
}

// 認証情報を取得する
function auth_get(rowid) {
	var logons = local_current();
	var auths = storage_get(logons["localid"], logons["localpass"]).split("\r\n");
	var auth = "";
	var bufs;
	var i;
	
	for(i = 0; i < auths.length; i++) {
		bufs = auths[i].split("\t");
		if(rowid == bufs[0]) {
			auth = auths[i];
			break;
		}
	}
	
	return auth;
}

// 認証情報をパースする
function auth_parse(auth) {
	var rets = new Array();
	var rownum = -1;
	var rowid = "";
	var fiid = "";
	var keyvalues = new Array();
	var lists;
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

// 認証情報をソートする
function auths_sort(auths) {
	var logons = local_current();
	var rets = new Array();
	var settings;
	var rowid, auth, ofx;
	var i, j, k, l;
	
	// authsをfiids[]の登録順でソートする
	k = 0;
	l = -1;
	for(i in fiids) for(j = 0; j < auths.length; j++) {
		settings = auth_parse(auths[j]);
		if(settings["fiid"] == i) {
			// fiidが一致した場合
			if(settings["rownum"] != k) {
				// ソート順が崩れた場合
				if(l == -1) l = k;
				rowid = (settings["rownum"] == -1? k.toString() + settings["rowid"]: settings["rowid"].replace(settings["rownum"].toString(), k.toString()));
				auth = auths[j].replace(settings["rowid"], rowid);
			} else {
				auth = auths[j];
			}
			rets.push(auth);
			k++;
		}
	}
	
	if(l != -1) {
		if(settings["rownum"] == -1) {
			// 追加の場合、OFXを後から前に送る
			for(k = rets.length - 1; k >= l; k--) {
				settings = auth_parse(rets[(k == rets.length - 1? k: k + 1)]);
				rowid = settings["rowid"].replace(settings["rownum"].toString(), k.toString());
				// alert("+" + rowid + ":" + settings["rowid"]);
				
				ofx = storage_get(logons["localid"] + ":" + rowid, logons["localpass"]);
				if(ofx != null && ofx != "") {
					// OFXを削除・登録する
					storage_set(logons["localid"] + ":" + settings["rowid"], ofx, logons["localpass"]);
					storage_delete(logons["localid"] + ":" + rowid, logons["localpass"]);
				}
			}
		} else {
			// 削除の場合、OFXを前から後に送る
			for(k = l; k <= rets.length; k++) {
				settings = auth_parse(rets[(k == 0? k: k - 1)]);
				rowid = settings["rowid"].replace(settings["rownum"].toString(), (k == 0? k + 1: k).toString());
				// alert("-" + rowid + ":" + settings["rowid"]);
				
				ofx = storage_get(logons["localid"] + ":" + rowid, logons["localpass"]);
				if(ofx != null && ofx != "") {
					// OFXを削除・登録する
					storage_set(logons["localid"] + ":" + settings["rowid"], ofx, logons["localpass"]);
					storage_delete(logons["localid"] + ":" + rowid, logons["localpass"]);
				}
			}
		}
	}
	
	return rets;
}

// ログオン情報を追加する
function logoninfo_add(auth) {
	var logons = local_current();
	var lists;
	var settings;
	var enc;
	
	// ログオン情報を追加する
	enc = storage_get(logons["localid"], logons["localpass"]);
	lists = enc.split("\r\n");
	lists.push(auth);
	lists = auths_sort(lists);
	
	// ログオン情報を記憶する
	storage_set(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_list_all(lists);
	
	return;
}

// ログオン情報を更新する
function logoninfo_update(to, from) {
	var logons = local_current();
	var lists = new Array();
	var enc;
	
	// ログオン情報を更新する
	enc = storage_get(logons["localid"], logons["localpass"]);
	if(from != null && from.length != 0 && enc.indexOf(from) != -1) {
		enc = enc.replace(from, to);
		lists = enc.split("\r\n");
	}
	lists = auths_sort(lists);
	
	// ログオン情報を設定する
	storage_set(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_list_all(lists);
	
	return;
}

// ログオン情報を削除する
function logoninfo_delete(auth) {
	var logons = local_current();
	var lists = new Array();
	var settings = auth_parse(auth);
	var enc;
	
	// ログオン情報を削除する
	enc = storage_get(logons["localid"], logons["localpass"]);
	if(enc.indexOf(auth) != -1) {
		enc = enc.replace(auth, "");
		lists = enc.split("\r\n");
		
		// OFXを削除する
		storage_delete(logons["localid"] + ":" + settings["rowid"], logons["localpass"]);
	}
	lists = auths_sort(lists);
	
	// ログオン情報を記憶する
	storage_set(logons["localid"], lists.join("\r\n"), logons["localpass"]);
	
	fnc_list_all(lists);
	
	return;
}

// 合計の金額を更新する
function num_total_update() {
	var tag_tds = dom_get_tag("td", dom_get_tag("table")[0]);
	var total = 0;
	var i;
	
	for(i = 0; i < tag_tds.length; i++) with(tag_tds[i]) if(className == "balance" && firstChild.nodeValue != "") total += parseInt(firstChild.nodeValue.replace(/,/g, ""), 10);
	
	dom_get_id("num_total").firstChild.nodeValue = to_amount(total);
	
	return;
}

// 現在のログオン情報を取得する
function local_current() {
	var fiid = "logon";
	var inputs = fiids[fiid]["form"].split("|");
	var rets = new Array();
	var i;
	
	for(i = 0; i < inputs.length; i++) rets[inputs[i]] = storage_get(inputs[i]);
	
	return rets;
}

// モーダルウィンドウの未入力項目をチェックする
function form_empty_check() {
	var f = false;
	var fiid, inputs;
	var i;
	
	if(dom_get_id("fiid") != null) {
		// 認証情報の場合
		fiid = dom_get_id("fiid").value;
		
		// 入力項目を取得する
		inputs = fiids[fiid]["form"].split("|");
		for(i = 0; i < inputs.length; i++) with(dom_get_id(inputs[i])) if(type != "hidden" && value == "") {
			// 未入力の場合
			f = true;
			break;
		}
		if(dom_get_id("confirm") != null && dom_get_id("confirm").checked == false) f = true;
		
		// OKボタンの押下を制御する（未入力項目がある場合、OKボタンの押下を禁止する）
		dom_get_id("modalok").disabled = f;
	} else {
		// 入力項目を取得する
		inputs = dom_get_tag("input", dom_get_id("modal"));
		for(i = 0; i < inputs.length; i++) with(inputs[i]) if(type != "hidden" && value == "") {
			// 未入力の場合
			f = true;
			break;
		}
		if(dom_get_id("confirm") != null && dom_get_id("confirm").checked == false) f = true;
		
		// OKボタンの押下を制御する（未入力項目がある場合、OKボタンの押下を禁止する）
		dom_get_id("modalok").disabled = f;
	}
	
	return;
}

// blobをファイルダウンロードする
function filedownload(blob, filename, filetype) {
	var tag_section = dom_get_tag("section")[0];
	var tag_a;
	
	if(self.window.navigator.msSaveOrOpenBlob) {
		self.window.navigator.msSaveOrOpenBlob(blob, filename);
	} else {
		tag_a = dom_create_tag("a", { "href": (self.window.URL || self.window.webkitURL).createObjectURL(blob), "id": "filedownload", "type": filetype, "download": filename });
		tag_a.appendChild(dom_create_text("ダウンロード"));
		tag_section.appendChild(tag_a);
		dom_get_id("filedownload").click();
		tag_section.removeChild(tag_a);
	}
	
	return;
}


// =========================================================================
// 関数
// =========================================================================

// 全角英数字と全角記号の一部を半角文字に変換する（UTF-8）
function utf8_zentohan(str) {
	var fnc = function(str) {
		return String.fromCharCode(str.charCodeAt(0) - 0xFEE0);
	};
	
	return str.replace(/[！-～]/g, fnc).replace(/　/g, " ");
}

// 全角・半角を考慮し、文字列strを全角zenlen文字ずつ切り出す（UTF-8）
function utf8_split(str, zenlen) {
	var rets = new Array();
	var bytes = new Array();
	var buf = "";
	var zensize = 2;
	var hanlen = zenlen * zensize;
	var i, j, k, l;
	
	for(i = 0; i < str.length; i++) bytes.push(encodeURI(str.charAt(i)).replace(/%[0-9A-Fa-f]{2}/g, String.fromCharCode(0x00)).length > 1? zensize: 1);
	
	k = 0;
	l = 0;
	for(var i = 0; i < bytes.length; i++) {
		j = bytes[i];
		k += j;
		if(k > hanlen) {
			rets.push(buf);
			k = j;
			buf = "";
		}
		buf += str.charAt(l);
		l++;
	}
	if(buf != "") rets.push(buf);
	
	return rets;
}

// 現在時刻をYYYYMMDDHHIISS形式で取得する
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

// 金額を3桁カンマ区切り文字に変換する
function to_amount(src) {
	var dst = src.toString().replace(/,/g, "");
	
	if(isNaN(Number(dst)) == true) dst = ""; else while(dst != (dst = dst.replace(/^(-?\d+)(\d{3})/, "$1,$2")));
	
	return dst;
}

// UTF-8文字列をShift_JISの16進数文字列に変換する
function get_binary_sjis(buf) {
	var buf_sjis = Encoding.convert(Encoding.stringToCode(buf), "SJIS", "UNICODE");
	var ret = "";
	var i;
	
	for(i = 0; i < buf_sjis.length; i++) {
		code = buf_sjis[i].toString(16);
		if(code.length == 1) code = "0" + code;
		ret += code;
	}
	
	return ret;
}

// JavaScriptの実装状況をチェックする（Ajax）
function chkenv_xmlhttprequest() {
	return (typeof XMLHttpRequest != "undefined" && XMLHttpRequest? true: false);
}

// JavaScriptの実装状況をチェックする（WebStorage）
function chkenv_webstorage() {
	var ret = false;
	if(self.window.localStorage && self.window.sessionStorage) try {
		// iOSのSafariブラウザーのプライベートブラウズモード対策
		with(self.window.localStorage) {
			setItem("undefined", "");
			removeItem("undefined");
		}
		ret = true;
	} catch(e) {
		void(0);
	}
	return ret;
}

// JavaScriptの実装状況をチェックする（Blob）
function chkenv_blob() {
	return (typeof Blob == "function" && Blob? true: false);
}

// JavaScriptの実装状況をチェックする（OFX解析）
function chkenv_domparser() {
	return (typeof DOMParser != "undefined" && DOMParser && typeof (new DOMParser).parseFromString == "function"? true: false);
}

// JavaScriptの実装状況をチェックする（OFX/CSV生成）
function chkenv_xmlserializer() {
	return (typeof XMLSerializer != "undefined" && XMLSerializer && typeof (new XMLSerializer).serializeToString == "function"? true: false);
}

// JavaScriptの実装状況をチェックする（CSV処理）
function chkenv_arraybuffer() {
	return (typeof ArrayBuffer == "function" && ArrayBuffer && typeof Uint8Array == "function" && Uint8Array? true: false);
}

// JavaScriptの実装状況をチェックする（ファイルダウンロード）
function chkenv_createobjecturl() {
	return ((self.window.URL || self.window.webkitURL) && ((self.window.URL || self.window.webkitURL).createObjectURL || self.window.navigator.msSaveOrOpenBlob)? true: false);
}

// JavaScriptの実装状況をチェックする（FileReader）
function chkenv_filereader() {
	return (self.window.File && self.window.FileReader? true: false);
}

// JavaScriptの実装状況をチェックする（OFX処理）
function chkenv_parser() {
	return chkenv_domparser() && chkenv_xmlserializer();
}

// JavaScriptの実装状況をチェックする（実行）
function chkenv_run() {
	return chkenv_xmlhttprequest() && chkenv_webstorage() && chkenv_parser();
}

// JavaScriptの実装状況をチェックする（登録機能）
function chkenv_import() {
	return chkenv_blob() && chkenv_filereader();
}

// JavaScriptの実装状況をチェックする（OFXダウンロード機能・口座情報エクスポート機能）
function chkenv_export() {
	return chkenv_blob() && chkenv_createobjecturl();
}

// JavaScriptの実装状況をチェックする（OFX結合ダウンロード機能）
function chkenv_ofx_all() {
	return chkenv_parser() && chkenv_blob() && chkenv_createobjecturl();
}

// JavaScriptの実装状況をチェックする（CSVダウンロード機能）
function chkenv_csv() {
	return chkenv_parser() && chkenv_blob() && chkenv_createobjecturl() && chkenv_arraybuffer();
}

// JavaScriptの実装状況をチェックする（PDFダウンロード機能）
function chkenv_pdf() {
	return chkenv_blob() && chkenv_createobjecturl() && chkenv_arraybuffer();
}

// DOMよりタグに合致するエレメントを取得する
function dom_get_tag(name, from) {
	var obj = null;
	
	if(typeof from != "object") from = self.document;
	
	if(typeof self.document.getElementsByTagName != "undefined") try {
		if(typeof name == "string") obj = from.getElementsByTagName(name);
	} catch(e) {
		void(e);
	}
	
	return obj;
}

// DOMよりIDに合致するエレメントを取得する
function dom_get_id(name, from) {
	var obj = null;
	
	if(typeof from != "object") from = self.document;
	
	if(typeof self.document.getElementById != "undefined") try {
		if(typeof name == "string") obj = from.getElementById(name);
	} catch(e) {
		void(e);
	}
	
	return obj;
}

// DOMのテキストを生成する
function dom_create_text(text) {
	var obj = null;
	
	with(self.document) if(typeof createTextNode != "undefined") try {
		if(typeof text == "string") obj = createTextNode(dom_convert_escape(text));
	} catch(e) {
		void(e);
	}
	
	return obj;
}

// DOMのタグを生成する
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

// DOMの特定文字をエスケープする
function dom_convert_escape(str) {
	var ret = "";
	var buf = "";
	var hcs = { "amp": "&", "quot": "\"", "lt": "<", "gt": ">" };
	var fnc;
	var i;
	
	if(str.indexOf(hcs["amp"]) == -1) {
		ret = str;
	} else {
		fnc = function() {
			return hcs[arguments[1]];
		};
		
		for(i in hcs) buf += "|" + i;
		ret = str.replace(new RegExp(hcs["amp"] + "(" + buf.substring(1) + ");", "g"), fnc);
	}
	
	return ret;
}
