var sheet = null;

$(document).ready(function() {
	$('a').click(load);
	$.get('../xsl/lab.xsl', function(data) {
		sheet = data;
	});
});

function load() {
	var link = $(this).attr('href');
	if (!link.match(/^(?:http|\.\.)/)) {
		location.hash = encodeURI(link);
		link = 'bbs/' + link;
	}
	$('#status').removeClass('st_err').addClass('st_load').html('<img src="images/indicator.gif"/>Loading').fadeIn('fast');
	$.get(link, function(data) {
		if (typeof data == 'object') {
			var x = new xslt(sheet);
			$('#main').empty().append(x.transform(data));
			$('#main a').click(load);
			$('#status').hide();
		} else {
			$('#status').addClass('st_err').html($(data).filter('div').text());
		}
		$('#status').removeClass('st_load');
	});
	return false;
}

var xslt = function(stylesheet) {
	if (typeof XSLTProcessor != 'undefined') {
		this.processor = new XSLTProcessor();
		this.processor.importStylesheet(stylesheet);
	}
};

xslt.prototype.transform = function(xml) {
	if (this.processor) {
		return this.processor.transformToFragment(xml, document);
	} else if ('transformNode' in node) {
		return xml.transformNode(sheet);
	} else {
		throw "xslt not supported"
	}
};

function xslt(xml, xsl) {
	var processor = null;
	var result;
	if (typeof XSLTProcessor != "undefined") {
		processor = new XSLTProcessor();
		processor.importStylesheet(xsl);
		result = processor.transformToFragment(xml, document);
	} else if ("transformNode" in xml) {
		xml.transformNode(xsl);
	} else {
		throw "XSLT not supported";
	}
	return result;
}
