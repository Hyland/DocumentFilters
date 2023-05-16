/*
(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// namespace
var perceptive = perceptive || {};

perceptive.viewer = function(ele) {
	this._ele = ele;
	this._rotation = this._pagesLoading = 0;
	this._scale = 1;
	this._pagesLoadedEvent = [];
	if (this._ele == null)
		this._ele = $('.idf-background');
}
perceptive.viewer.prototype = {
	_rotation: 0,
	_scale: 1,
	_pagesLoading: 0,
	_pagesLoadedEvent: [],
	prepare: function() {
		var me = this;
		this._viewPort = $(window);
		this._pages = [];
		this._scale = 1.0;
		this.pages().each(function(index, e) {
			var ele = $(e);

			var scale = $('<div class="div-scale">');
			var rotate = $('<div class="div-rotate">');
			me._ele.append(scale);
			scale.append(rotate);
			ele.appendTo(rotate);

			me._pages[index] = { container: scale, rotation: rotate, element: ele, width: ele.width(), height: ele.height(), ready: ele.children().length != 0 };
		});
		this._viewPort.on('scroll', function() { me.onScroll(); });
		return this;
	},
	installToolbar: function() {
		this._toolbar = $('<div>').addClass('perceptive-viewer-toolbar');
		this._ele.prepend(this._toolbar);

		var pageMenu = [];
		var pageCount = this.pages().length;
		for (var i = 0; i < pageCount; i++)
			pageMenu[i] = { text: "Page " + (i+1), value: i };

		this
			.addToolbarButton({text: 'Page', rightIcon: 'fa-sort-down', id: 'toolbar-page-menu', items: pageMenu, event: this.onGotoPage})
			.addToolbarButton({icon: 'fa-home', event: this.gotoFirstPage, tooltip: 'First page'})
			.addToolbarButton({icon: 'fa-chevron-up', event: this.gotoPrevPage, tooltip: 'Previous page'})
			.addToolbarButton({icon: 'fa-chevron-down', event: this.gotoNextPage, tooltip: 'Next page'})
			.addToolbarSpacer()
			.addToolbarButton({icon: 'fa-print', event: this.print, tooltip: 'Print'})
			.addToolbarSpacer()
			.addToolbarButton({icon: 'fa-rotate-left', event: this.rotateLeft, tooltip: 'Rotate left'})
			.addToolbarButton({icon: 'fa-rotate-right', event: this.rotateRight, tooltip: 'Rotate right'})
			.addToolbarButton({icon: 'fa-paint-brush ', event: this.highlightText, tooltip: 'Highlight text'})
			.addToolbarButton({icon: 'fa-eraser ', event: this.redactText, tooltip: 'Redact text'})
			.addToolbarSpacer()
			.addToolbarButton({icon: 'fa-search-plus', event: this.zoomIn, tooltip: 'Zoom in'})
			.addToolbarButton({icon: 'fa-search-minus', event: this.zoomOut, tooltip: 'Zoom out'})
			.addToolbarButton({icon: 'fa-file-o', event: this.zoomReset, tooltip: 'Zoom 100%'})
			.addToolbarSpacer()
			.addToolbarButton({icon: 'fa-info-circle', event: this.toogleDetails, tooltip: 'Details'});

		this.onScroll();
		return this;
	},
	installDetailsPanel: function() {
		var meta = $('meta');
		if (meta.length > 1)
		{
			this._details = $('<div>').addClass('perceptive-viewer-details');
			this._details.append('<div class="section">Metadata</div>');
			for (var i = 0; i < meta.length; i++)
			{
				if (meta[i].name)
				{
					var item = $('<div class="item">');
					$('<div class="caption">').text(meta[i].name + ': ').appendTo(item);
					$('<div class="value">').text(meta[i].content).appendTo(item);
					$('<div style="clear: both">').appendTo(item);
					item.appendTo(this._details);
				}
			}
			this._ele.prepend(this._details);
		}
		return this;
	},
	onScroll: function() {
		$('#toolbar-page-menu').find('span').text('Page ' + (this.pageIndex()+1));
		this._loadPagesAsRequired();
	},
	onGotoPage: function(e) {
		this.gotoPage(parseInt(e.selected));
	},
	addToolbarButton: function(info) {
		var h = '<div class="perceptive-viewer-toolbar-button">';
		if (info.icon)      h += '<i class="fa ' + info.icon + '" /> ';
		if (info.text)      h += '<span class="caption">' + info.text + '</span> ';
		if (info.rightIcon) h += '<i class="fa ' + info.rightIcon + '" />';
		h += '</div>'
		
		var me = this;
		var button = $(h);
		if (info.id)
			button.attr('id', info.id);
		if (info.tooltip)
			button.append($('<div>').addClass('perceptive-viewer-toolbar-tip').text(info.tooltip));
		if (info.items)
		{
			var s = $('<select>');
			for (var i = 0; i < info.items.length; i++)
			{
				if (typeof(info.items[i]) == 'string')
					s.append($('<option>').text(info.items[i]));
				else
					s.append($('<option>').text(info.items[i].text).attr('value', info.items[i].value));
			}
			button.append($(s));
			if (info.event)
			{
				s.on('change', function(e) {
					e.selected = s.val(); 
					$.proxy(info.event, me, e)();
				});
			}
		}
		else if (info.event)
			button.on('mousedown', function(e) { e.preventDefault(); });
			button.on('mouseup', function(e) { e.preventDefault(); });
			button.on('click', function(e) { 
				e.preventDefault();
				$.proxy(info.event, me, e)(); 
			});

		this._toolbar.append(button);		
		return this;
	},	
	addToolbarMenu: function(info) {
		var button = $('<div class="perceptive-viewer-toolbar-menu"><span class="caption">' + info.text + '</span> <i class="fa fa-sort-down" /></div>');
		if (id)
			button.attr("id", id);
		//button.append($('<div>').addClass('perceptive-viewer-toolbar-tip').text(tooltip));
		button.on('click', $.proxy(event, this));
		this._toolbar.append(button);

		return this;
	},
	addToolbarSpacer: function() {
		this._toolbar.append($('<div class="perceptive-viewer-toolbar-spacer"></div>'));
		return this;
	},	
	pageIndex: function(val) {
		var pages=this.pages();
		var h = $(window).height() || this._ele.height();
		var y = this._viewPort.scrollTop();
		var ret = { page: 0, size: 0 };
		for (var i=0; i < pages.length; i++) {
			var pageY = $(pages[i]).position().top;
			var pageHeight = $(pages[i]).height();

			var vis = 0;
			if ( (pageY >= y && pageY <= y + h) || (pageY + pageHeight >= y && pageY + pageHeight <= y + h) || (pageY < y && pageY + pageHeight > y+h) )
			{
				var top = Math.max(pageY, y);
				var bottom = Math.min(pageY + pageHeight, y + h);
				var vis = bottom-top;
				if (vis > ret.size)
				{
					ret.size = vis;
					ret.page = i;
				}
			}
		}
		return ret.page;
	},
	pages: function() {
		return this._ele.find('.idf-page');
	},
	gotoFirstPage: function() {
		this._viewPort.scrollTop(0);
	},
	gotoNextPage: function() {
		var pages=this.pages();
		var x = this.pageIndex() + 1;
		if (x < pages.length)
		{
			//this._ele.scrollTop($(pages[x]).offset().top - 75);
			this._viewPort.scrollTop($(pages[x]).offset().top - 75);
		}
	},
	gotoPrevPage: function() {
		var pages=this.pages();
		var x = this.pageIndex() - 1;
		if (x >= 0)
		{
			this._viewPort.scrollTop($(pages[x]).offset().top - 75);
		}
	},
	gotoPage: function(index) {
		var pages=this.pages();
		if (index >= 0 && index < pages.length)
		{
			this._viewPort.scrollTop($(pages[index]).offset().top - 75);
		}
	},
	print: function() {
		this.loadAllPages(function() { window.print(); });
	},
	loadAllPages: function(callback) {
		var me = this;
		var ret = 0;
		this.pages().each(function(index, p) {
			var page = me._pages[index];
			if (page.element.attr('title') != '')
				if (me._loadPageData(index)) ret++;
		});

		if (callback)
		{
			if (ret == 0) 
				callback();		// All pages are already loaded
			else
				me._pagesLoadedEvent[me._pagesLoadedEvent.length] = callback;
		}
		return ret;
	},
	rotateLeft: function() {
		this._rotate(-90);
	},
	rotateRight: function() {
		this._rotate(+90);
	},
	zoomIn: function() {
		this._zoom(+0.10)
	},
	zoomOut: function() {
		this._zoom(-0.10)	
	},
	zoomReset: function() {
		this._scale = 1.0;
		this._setTransforms();
	},
	toogleDetails: function() {
		if (this._details.css('display') == 'none')
			this._details.css('display', 'block');
		else
			this._details.css('display', 'none');
	},
	enumSelection: function(callback) {
		var info = { found: false, range: window.getSelection().getRangeAt(0) };

		var range = window.getSelection().getRangeAt(0);
		var proc = function(info, n, depth) {
			var node = { element: n, full: false, depth: depth };

			if (n.contains && !n.contains(range.endContainer) && !n.contains(range.startContainer))
				node.full = true;

			if (n == range.startContainer)
			{
				info.found = true;
				node.startOffset = range.startOffset;
			}
			if (n == range.endContainer) 
				node.endOffset = range.endOffset;

			if (info.found)
				callback(node, info);

			for (var c = n.firstChild; c != null; c = c.nextSibling)
			{
				proc(info, c, depth+1);
			}

			if (n == range.endContainer)
				info.found = false;
		};
		if (range.commonAncestorContainer)
			proc(info, range.commonAncestorContainer, 0);
	},
	highlightText: function() {
		var color = 'rgba(255,255,0,0.5)';

		
		this.enumSelection(function(node) {
			if ($(node.element.parentNode).parents('.idf-page').length == 0)
				return;
			if ((node.element.className == "idf-p" || node.element.className == "idf-r") && node.full)
			{
				$(node.element).css('background-color', color);
			}
			else if (node.element.nodeType == 3 && node.startOffset && node.endOffset)
			{
				// Partial text node
				var t = node.element.textContent;
				node.element.textContent = t.substring(0, node.startOffset);
				$('<span>')
					.text(t.substring(node.startOffset, node.endOffset))
					.css('background-color', color)
					.appendTo(node.element.parentNode);
				$('<span>')
					.text(t.substr(node.endOffset))
					.appendTo(node.element.parentNode);						
			}
			else if (node.element.nodeType == 3 && node.startOffset)
			{
				// Partial text node
				var t = node.element.textContent;
				node.element.textContent = t.substr(0, node.startOffset);
				$('<span>')
					.text(t.substr(node.startOffset))
					.css('background-color', color)
					.appendTo(node.element.parentNode);
			}
			else if (node.element.nodeType == 3 && node.endOffset)
			{
				// Partial text node
				var t = node.element.textContent;
				node.element.textContent = t.substr(node.endOffset);
				$('<span>')
					.text(t.substr(0, node.endOffset))
					.css('background-color', color)
					.prependTo(node.element.parentNode);
			}
		});
		this.clearSelection();
	},
	_parseXwords: function(text, startOffset, endOffset) {
		var p = new String(text).split(' ');
		if (p[0] != 'x-words' || p.length < 2)
			return [];

		var w = parseInt(p[1]), x = 0, len = 0, nfo = [];

		for (var i = 2; i < p.length; i++)
		{
			len = parseInt(p[i]);
			nfo[nfo.length] = { word: w++, index: x, len: len };
			x += len;
		}

		var ret = [];
		startOffset = startOffset || 0;
		endOffset = endOffset || 0xffff;

		for (var i = 0; i < nfo.length; i++)
		{
			if (startOffset < nfo[i].index + nfo[i].len && endOffset > nfo[i].index)
				ret[ret.length] = nfo[i].word;
		}
		return ret;
	},
	redactText: function() {
		var me = this;
		var results = {};
		this.enumSelection(function(node) {
			var page = $(node.element.parentNode).parents('.idf-page');
			if (page.length == 0)
				return;

			var pageIndex = me.pages().index(page);
			results[pageIndex] = results[pageIndex] || [];

			if ((node.element.className == "idf-r") && node.full)
			{
				results[pageIndex] = results[pageIndex].concat(me._parseXwords($(node.element).attr('title'))).filter(function(item, pos, self) { return self.indexOf(item) == pos; });
			}
			else if (node.element.nodeType == 3)
			{
				// Partial text node
				results[pageIndex] = results[pageIndex].concat(me._parseXwords($(node.element).parents('.idf-r').attr('title'), node.startOffset, node.endOffset)).filter(function(item, pos, self) { return self.indexOf(item) == pos; });
			}
		});
		
		// Results now contains an array of word indexes for pages within the selection, this can be sent to the server for redaction processing.
		var msg = 'This demo does not apply the actual redaction, it must be performed on a server. The redaction information is: \n\n';
		for (var p in results)
		{
			msg += 'Page ' + (parseInt(p)+1) + ' - words ' + this._arrayOfNumbersToString(results[p]) + '\n';
		}

		window.alert(msg);
	},
	clearSelection: function() {
		if (window.getSelection) {
			if (window.getSelection().empty)window.getSelection().empty();
			else if (window.getSelection().removeAllRanges) window.getSelection().removeAllRanges();
		}
		else if (document.selection) document.selection.empty();
	},
	_arrayOfNumbersToString: function(arr) {
		var ret = '';
		arr = arr.sort(function(a,b){return a-b});

		if (arr.length)
		{
			ret = arr[0]+'';
			var c = 0, i = 0;
			for (i = 1; i <= arr.length; i++)
			{
				if (i == arr.length || arr[i]-arr[i-1]!=1)
				{
					if (c > 0)
						ret += '..' + arr[i-1];
					else
						ret += ',' + arr[i-1];
					if (i < arr.length) ret += arr[i];
					c = 0;
				}
				else
					c++;			
			}

		}
		return ret;
	},
	_setTransforms: function() {
		var me = this;
		this.pages().each(function(index, ele) {
			var e = $(ele);
			var rot = "rotate(" + me._rotation + "deg)";
			var sca = "scale(" + me._scale + "," + me._scale + ")";
			
			var rotorg =  '', scaorg = '';
			if (me._rotation == 90)  {
				rotorg = '50% 50%';
				scaorg = '-' +  Math.floor(50 * me._scale) + '% 50%';
			}
			else if (me._rotation == 270) {
				scaorg =  '100% 50%';	
			}
			else if (me._rotation == 180) {
				rotorg = '50% ' + Math.floor(50 * me._scale) + '%';
				scaorg = '50% -10%'
			}
			else 
			{
				scaorg = '50% 0px'
			}

			e.css("-ms-transform", sca).css("-webkit-transform", sca).css("transform", sca).css("-moz-transform", sca).css('transform-origin', scaorg); 
			me._pages[index].rotation.css("-ms-transform", rot).css("-webkit-transform", rot).css("-moz-transform", rot).css("transform-origin", rotorg);
			if (me._rotation == 90 || me._rotation == 270)
				me._pages[index].container.height((me._pages[index].width + 25) * me._scale);
			else
				me._pages[index].container.height((me._pages[index].height + 25) * me._scale);

		})
	},
	_rotate: function(amount) {
		this._rotation += amount;
		if (this._rotation < 0)
			this._rotation = 270;
		else if (this._rotation >= 360)
			this._rotation = 0;

		this._setTransforms();
	},
	_zoom: function(amount) {
		var me = this;
		me._scale += amount;
		if (me._scale < 0.3) me._scale = 0.3;
		if (me._scale > 2.5) me._scale = 2.5;
		this._setTransforms();
	},
	_loadPageData: function(page) {
		var me = this;
		var p = this._pages[page];
		if (p.element.attr('title') && !p.ready)
		{
			me._pagesLoading++;
			p.element.html('<div class="async-loading"><i class=" fa fa-circle-o-notch fa-spin fa-3x"></i></div>');

			$.get(this._pages[page].element.attr('title'), null, function(data) {
				var d = null;
				if (data.documentElement && data.documentElement.outerHTML)
					d = $(data.documentElement.outerHTML);
				else
					d = $(data);
				p.element.html(d.children());
			}, 'html').fail(function(e) {
				if (window.location.protocol == "file:")
					p.element.html('<div class="error">Cannot load pages asynchronously when opened from file://.<br />Please view through a http connection.</div>');
				else
					p.element.html('<div class="error">Failed to download page data</div>');
			}).always(function() {
				me._pagesLoading--;
				if (me._pagesLoading == 0)
				{
					$.each(me._pagesLoadedEvent, function(index, val) { val(); });
					me._pagesLoadedEvent = [];
				}
				p.ready = true;
			});
			return true;
		}
		else
			return false;
	},
	_loadPagesAsRequired: function() {
		var pages=this.pages();
		var h = $(window).height() || this._ele.height();
		var y = this._viewPort.scrollTop();
		var ret = { page: 0, size: 0 };
		for (var i=0; i < pages.length; i++) {
			var pageY = $(pages[i]).position().top;
			var pageHeight = $(pages[i]).height();

			var vis = 0;
			if ( (pageY >= y && pageY <= y + h) || (pageY + pageHeight >= y && pageY + pageHeight <= y + h) || (pageY < y && pageY + pageHeight > y+h) )
			{
				var top = Math.max(pageY, y);
				var bottom = Math.min(pageY + pageHeight, y + h);
				var vis = bottom-top;
				this._loadPageData(i);
			}
		}
		return ret.page;
	}
	

};