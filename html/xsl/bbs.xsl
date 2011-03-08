<?xml version='1.0' encoding='gb2312'?>
<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:import href='showpost.xsl'/>
<xsl:output method='html' encoding='gb2312' indent='no' doctype-public='-//W3C//DTD HTML 4.01//EN' doctype-system='http://www.w3.org/TR/html4/strict.dtd'/>

<xsl:template name='timeconvert'>
	<xsl:param name='time'/>
	<xsl:value-of select='concat(substring($time, 1, 10), " ", substring($time, 12, 5))'/>
</xsl:template>

<xsl:template name="splitbm">
	<xsl:param name='names'/>
	<xsl:param name='isdir'/>
	<xsl:param name='isfirst'/>
	<xsl:variable name='first' select='substring-before($names," ")'/>
	<xsl:variable name='rest' select='substring-after($names," ")'/>
	<xsl:if test='$first'>
		<a href='qry?u={$first}'><xsl:value-of select='$first'/></a>
	</xsl:if>
	<xsl:if test='$rest'>
		<xsl:text>&#160;</xsl:text>
		<xsl:call-template name='splitbm'>
			<xsl:with-param name='names' select='$rest'/>
			<xsl:with-param name='isdir' select='$isdir'/>
			<xsl:with-param name='isfirst' select='0'/>
		</xsl:call-template>
	</xsl:if>
	<xsl:if test='not($rest)'>
		<xsl:if test='$names'>
			<a href='qry?u={$names}'><xsl:value-of select='$names'/></a>
		</xsl:if>
		<xsl:if test="$names=''">
			<xsl:if test="$isdir='0'">����������</xsl:if>
			<xsl:if test="$isdir!='0'">-</xsl:if>
		</xsl:if>
	</xsl:if>
</xsl:template>

<xsl:template name='navigation'>
	<xsl:param name='session'/>
	<xsl:variable name='bbsname'><xsl:call-template name='bbsname'/></xsl:variable>
	<ul id='nav'>
		<li id='navh'><a href='sec'>�Ƽ�����</a></li>
		<xsl:if test='$bbsname="���¹⻪"'><li id='navb'>
			<a href='#'>��������</a>
			<ul><li><a href='boa?s=0'>0 BBSϵͳ</a></li>
			<li><a href='boa?s=1'>1 ������ѧ</a></li>
			<li><a href='boa?s=2'>2 Ժϵ���</a></li>
			<li><a href='boa?s=3'>3 ���Լ���</a></li>
			<li><a href='boa?s=4'>4 ��������</a></li>
			<li><a href='boa?s=5'>5 ��ѧ����</a></li>
			<li><a href='boa?s=6'>6 ��������</a></li>
			<li><a href='boa?s=7'>7 ���Կռ�</a></li>
			<li><a href='boa?s=8'>8 ������Ϣ</a></li>
			<li><a href='boa?s=9'>9 ѧ��ѧ��</a></li>
			<li><a href='boa?s=A'>A ����Ӱ��</a></li>
			<li><a href='boa?s=B'>B ����ר��</a></li>
			</ul>
		</li></xsl:if>
		<li id='nava'><a href='0an'>��վ����</a></li>
		<li id='navp'><a href='all'>ȫ������</a></li>
		<li id='navt'>
			<a href='#'>ͳ������</a>
			<ul>
				<li><a href='top10'>����ʮ��</a></li>
			</ul>
		</li>
		<xsl:if test='contains($session/p, "l")'>
			<li id='navf'>
				<a href='#'>�ҵ��ղ�</a>
				<ul>
					<li><a href='fav'>�鿴����</a></li>
					<xsl:for-each select='$session/f/b'><xsl:sort select='.'/>
						<li class='sf'><a href='{$session/@m}doc?board={.}'><xsl:value-of select='.'/></a></li>
					</xsl:for-each>
				</ul>
			</li>
			<li id='navc'>
				<a href='#'>ȵ�����</a>
				<ul>
					<li><a href='ovr'>���ߺ���</a></li>
				</ul>
			</li>
			<li id='navm'>
				<a href='#'>�����ż�</a>
				<ul>
					<li><a href='newmail'>��������</a></li>
					<li><a href='mail'>�����ż�</a></li>
					<li><a href='pstmail'>�����ż�</a></li>
				</ul>
			</li>
			<li id='navco'>
				<a href='#'>��������</a>
				<ul>
					<li><a href='info'>��������</a></li>
					<li><a href='plan'>��˵����</a></li>
					<li><a href='sig'>��ǩ����</a></li>
					<li><a href='pwd'>�޸�����</a></li>
					<li><a href='fall'>�趨����</a></li>
				</ul>
			</li>
		</xsl:if>
		<li id='navs'>
			<a href='#'>����ѡ��</a>
			<ul>
				<li><a href='qry'>��ѯ����</a></li>
				<li><a href='sel'>���Ұ���</a></li>
			</ul>
		</li>
	</ul>
</xsl:template>

<xsl:template name='header'>
	<xsl:param name='session'/>
	<xsl:variable name='user' select='string($session/u)'/>
	<div id='hd'>
		<div id='hdright'><xsl:if test='$user != ""'><a id='nave' href='logout'>ע��</a></xsl:if></div>
		<xsl:if test='$user != ""'><a id='navu' href='qry?u={$user}'><xsl:value-of select='$user'/></a></xsl:if>
		<xsl:if test='$user = ""'><a id='navl' href='login'>��¼</a></xsl:if>
		<a id='navnm' href='newmail'>����[<span id='navmc'></span>]�����ż�</a>
		<a id='navte' href='telnet://bbs.fudan.sh.cn:23'>�ն˵�¼</a>
		<span id='iewarn'><xsl:comment><![CDATA[[if lt IE 7]><![endif]]]></xsl:comment></span>
	</div>
</xsl:template>

<xsl:template name='foot'>
	<div id='ft'><a href='#'>[<img src='../images/button/up.gif'/>��ҳ��]</a>&#160;<xsl:call-template name='bbsname'/> &#169;1996-2011 Powered by <a href='http://code.google.com/p/fbbs/'><strong>fbbs</strong></a></div>
</xsl:template>

<xsl:template name='bbsname'>���¹⻪</xsl:template>
<xsl:template name='include-css'>
	<link rel='stylesheet' type='text/css' href='../css/bbs.css?v1283'/>
	<xsl:comment><![CDATA[[if lt IE 7]><link rel='stylesheet' type='text/css' href='../css/ie6fix.css?v1283'/><![endif]]]></xsl:comment>
</xsl:template>
<xsl:template name='include-js'>
	<script src='../js/persist-all-min.js'></script>
	<script src='/js/jquery-1.5.1.min.js'></script>
	<script src='../js/bbs.js?v1283' charset='gb2312' defer='defer'></script>
</xsl:template>

<xsl:template name='page-title'>
	<xsl:variable name='cgi' select='local-name(node()[2])'/>
	<xsl:choose>
		<xsl:when test='bbssec'>�Ƽ�����</xsl:when>
		<xsl:when test='bbsboa'><xsl:choose><xsl:when test='bbsboa/@dir'>����Ŀ¼</xsl:when><xsl:otherwise>����������</xsl:otherwise></xsl:choose></xsl:when>
		<xsl:when test='bbsall'>ȫ��������</xsl:when>
		<xsl:when test='bbssel'>ѡ��������</xsl:when>
		<xsl:when test='bbsdoc'><xsl:value-of select='bbsdoc/brd/@desc'/></xsl:when>
		<xsl:when test='bbscon'>�����Ķ�</xsl:when>
		<xsl:when test='bbstcon'>ͬ���������Ķ�</xsl:when>
		<xsl:when test='bbsqry'>��ѯ����</xsl:when>
		<xsl:when test='bbspst'><xsl:choose><xsl:when test='bbspst/@edit="0"'>����</xsl:when><xsl:otherwise>�޸�</xsl:otherwise></xsl:choose>����</xsl:when>
		<xsl:when test='bbstop10'>����ʮ��</xsl:when>
		<xsl:when test='bbsbfind'>�������²�ѯ</xsl:when>
		<xsl:when test='bbsnewmail'>�������ż�</xsl:when>
		<xsl:when test='bbsmail'>�ż��б�</xsl:when>
		<xsl:when test='bbsmailcon'>�ż��Ķ�</xsl:when>
		<xsl:when test='bbspstmail'>�����Ÿ�</xsl:when>
		<xsl:when test='bbs0an'>���������</xsl:when>
		<xsl:when test='bbsanc'>�����������Ķ�</xsl:when>
		<xsl:when test='bbsfwd'>ת������</xsl:when>
		<xsl:when test='bbsccc'>ת������</xsl:when>
		<xsl:when test='bbsfall'>�趨��ע����</xsl:when>
		<xsl:when test='bbsfadd'>���ӹ�ע����</xsl:when>
		<xsl:when test='bbsovr'>���߹�ע����</xsl:when>
		<xsl:when test='bbsfav'>�ղؼ�</xsl:when>
		<xsl:when test='bbsmybrd'>�趨�ղؼ�</xsl:when>
		<xsl:when test='bbseufile'><xsl:value-of select='bbseufile/@desc'/></xsl:when>
		<xsl:when test='bbsinfo'>��������</xsl:when>
		<xsl:when test='bbspwd'>�޸�����</xsl:when>
		<xsl:when test='bbsnot'>���滭��</xsl:when>
		<xsl:when test='bbsreg'>ע���ʺ�</xsl:when>
		<xsl:otherwise></xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='/'>
	<html>
		<head>
			<title><xsl:call-template name='page-title'/> - <xsl:call-template name='bbsname'/></title>
			<meta http-equiv="content-type" content="text/html; charset=gb2312"/>
			<xsl:call-template name='include-css'/>
<xsl:comment><![CDATA[[if lt IE 7]><style>
#hd{position:absolute;top:0;left:144px;margin:0.5em;}
#main{position:absolute;top:32px;left:144px;margin:0.5em 0 0.5em 0}
#ft{display:none;margin:0.5em}
#main td{font-size:12px;height:1}
table.content{width:100%}
th.ptitle,td.ptitle{width:auto}
.content tr{line-height:24px}
table.post{width:100%}
</style><![endif]]]></xsl:comment>
			<xsl:call-template name='include-js'/>
		</head>
		<body>
			<a name='top'/>
			<xsl:call-template name='navigation'><xsl:with-param name='session' select='node()[2]/session'/></xsl:call-template>
				<xsl:call-template name='header'><xsl:with-param name='session' select='node()[2]/session'/></xsl:call-template>
			<div id='main'><xsl:apply-templates/></div>
			<xsl:call-template name='foot'/>
		</body>
		<xsl:comment><![CDATA[[if lt IE 7]><script type='text/javascript' defer='defer'>addLoadEvent(ie6fix);window.onresize=ie6fix;</script><![endif]]]></xsl:comment>
	</html>
</xsl:template>

<xsl:template match='bbssec'>
	<img src='../images/secbanner.jpg'/>
	<xsl:for-each select='sec'>
		<ul class='sec'>
			<li><a href='boa?s={@id}'><xsl:value-of select='@id'/>&#160;<xsl:value-of select='@desc'/></a></li>
			<ul class='brd'>
				<xsl:for-each select='brd'>
					<li><a href='doc?board={@name}'><xsl:value-of select='@desc'/></a></li>
				</xsl:for-each>
			</ul>
		</ul>
	</xsl:for-each>
</xsl:template>

<xsl:template match='bbsboa'>
	<h2><xsl:if test='@icon'><img src='{icon}'/></xsl:if><xsl:value-of select='@title'/></h2>
	<table class='content'>
		<tr><th class='no'>���</th><th class='read'>δ��</th><th class='no'>������</th><th class='title'>����������</th><th class='cate'>���</th><th class='desc'>��������</th><th class='bm'>����</th></tr>
		<xsl:for-each select='brd'><xsl:sort select='@title'/><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:value-of select='position()'/></td>
			<td class='read'><xsl:choose><xsl:when test='@read="0"'>��</xsl:when><xsl:otherwise>��</xsl:otherwise></xsl:choose></td>
			<td class='no'><xsl:value-of select='@count'/></td>
			<td class='title'><a class='title'><xsl:choose>
				<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title'/></xsl:attribute>[ <xsl:value-of select='@title'/> ]</xsl:when>
				<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@title'/></xsl:otherwise>
			</xsl:choose></a></td>
			<td class='cate'><xsl:choose><xsl:when test='@dir="1"'>[Ŀ¼]</xsl:when><xsl:otherwise><xsl:value-of select='@cate'/></xsl:otherwise></xsl:choose></td>
			<td class='desc'><a class='desc'><xsl:attribute name='href'><xsl:choose><xsl:when test='@dir="1"'>boa</xsl:when><xsl:otherwise>doc</xsl:otherwise></xsl:choose>?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@desc'/></a></td>
			<td class='bm'><xsl:call-template name='splitbm'><xsl:with-param name='names' select='@bm'/><xsl:with-param name='isdir' select='@dir'/><xsl:with-param name='isfirst' select='1'/></xsl:call-template></td>
		</tr></xsl:for-each>
	</table>
</xsl:template>

<xsl:template match='bbsall'>
	<h2>ȫ��������</h2>
	<p>[��������: <xsl:value-of select="count(brd)"/>]</p>
	<table class='content'>
		<tr><th class='no'>���</th><th class='title'>����������</th><th class='cate'>���</th><th class='desc'>��������</th><th class='bm'>����</th></tr>
		<xsl:for-each select='brd'>
			<xsl:sort select="@title"/>
			<tr>
				<xsl:attribute name='class'>
					<xsl:if test='position() mod 2 = 1'>light</xsl:if>
					<xsl:if test='position() mod 2 = 0'>dark</xsl:if>
				</xsl:attribute>
				<td class='no'><xsl:value-of select='position()'/></td>
				<td class='title'><a class='title'><xsl:choose>
					<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title'/></xsl:attribute>[ <xsl:value-of select='@title'/> ]</xsl:when>
					<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@title'/></xsl:otherwise>
				</xsl:choose></a></td>
				<td class='cate'><xsl:choose>
					<xsl:when test='@dir="1"'>[Ŀ¼]</xsl:when>
					<xsl:otherwise><xsl:value-of select='@cate'/></xsl:otherwise>
				</xsl:choose></td>
				<td class='desc'><a class='desc'><xsl:choose>
					<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@desc'/></xsl:when>
					<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@desc'/></xsl:otherwise>
				</xsl:choose></a></td>
				<td class='bm'>
					<xsl:call-template name='splitbm'>
						<xsl:with-param name='names' select='@bm'/>
						<xsl:with-param name='isdir' select='@dir'/>
						<xsl:with-param name='isfirst' select='1'/>
					</xsl:call-template>
				</td>
			</tr>
		</xsl:for-each>
	</table>
</xsl:template>

<xsl:template match='bbssel'>
	<fieldset><legend>ѡ��������</legend><form action='sel' method='get'>
		<label for='brd'>���������ƣ�</label>
		<input type='text' name='brd' size='20' maxlength='20'/><br/>
		<input type='submit' value='�ύ��ѯ'/>
	</form></fieldset>
	<xsl:if test='count(brd)!=0'>
		<p>������: <xsl:value-of select='count(brd)'/>������</p>
		<table class='content'>
			<tr><th class='title'>����������</th><th class='desc'>��������</th></tr>
			<xsl:for-each select='brd'><xsl:sort select="@title"/><tr>
				<xsl:attribute name='class'>
					<xsl:if test='position() mod 2 = 1'>light</xsl:if>
					<xsl:if test='position() mod 2 = 0'>dark</xsl:if>
				</xsl:attribute>
				<td class='title'><a class='title'><xsl:choose>
					<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title'/></xsl:attribute>[ <xsl:value-of select='@title'/> ]</xsl:when>
					<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@title'/></xsl:otherwise>
				</xsl:choose></a></td>
				<td class='desc'><a class='desc'><xsl:choose>
					<xsl:when test='@dir="1"'><xsl:attribute name='href'>boa?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@desc'/></xsl:when>
					<xsl:otherwise><xsl:attribute name='href'>doc?board=<xsl:value-of select='@title'/></xsl:attribute><xsl:value-of select='@desc'/></xsl:otherwise>
				</xsl:choose></a></td>
			</tr></xsl:for-each>
		</table>
	</xsl:if>
	<xsl:if test='notfound'>û���ҵ����������İ���</xsl:if>
</xsl:template>

<xsl:template match='bbsdoc'>
	<xsl:choose>
		<xsl:when test='brd/@banner'><img src='{brd/@banner}'/></xsl:when>
		<xsl:otherwise><h2><xsl:if test='brd/@icon'><img src='{brd/@icon}'/></xsl:if><a href='{brd/@link}doc?bid={brd/@bid}'><xsl:value-of select='brd/@desc'/> [<xsl:value-of select='brd/@title'/>]<xsl:if test='brd/@link = "g"'> - ��ժ��</xsl:if><xsl:if test='brd/@link = "t"'> - ����ģʽ</xsl:if></a></h2></xsl:otherwise>
	</xsl:choose>
	<div class='btop'>
		<a class='newpost' href='pst?bid={brd/@bid}'>��������</a>
		<a href='brdadd?bid={brd/@bid}'>�ղر���</a>
		<a href='not?board={brd/@title}'>���滭��</a>
		<span>����: <xsl:call-template name='splitbm'><xsl:with-param name='names' select='brd/@bm'/><xsl:with-param name='isdir'>0</xsl:with-param><xsl:with-param name='isfirst' select='1'/></xsl:call-template></span>
	</div>

	<div class='bnav'>
		<a href='javascript:location=location'><img src='../images/button/reload.gif'/>ˢ��</a>
		<xsl:if test='brd/@start > 1'>
			<xsl:variable name='prev'><xsl:choose><xsl:when test='brd/@start - brd/@page &lt; 1'>1</xsl:when><xsl:otherwise><xsl:value-of select='brd/@start - brd/@page'/></xsl:otherwise></xsl:choose></xsl:variable>
			<a href='{brd/@link}doc?bid={brd/@bid}&amp;start={$prev}'><img src='../images/button/up.gif'/>��һҳ</a>
		</xsl:if>
		<xsl:if test='brd/@total > brd/@start + brd/@page - 1'>
			<xsl:variable name='next'><xsl:value-of select='brd/@start + brd/@page'/></xsl:variable>
			<a href='{brd/@link}doc?bid={brd/@bid}&amp;start={$next}'><img src='../images/button/down.gif'/>��һҳ</a>
		</xsl:if>
		<a href='clear?board={brd/@title}&amp;start={brd/@start}'>���δ��</a>
		<form class='jump' method='get' action='{brd/@link}doc'><input type='hidden' name='bid' value='{brd/@bid}'></input><img src='../images/button/forward.gif'/>��ת��<input type='text' name='start' size='6'/>ƪ</form>
	</div>

	<table class='content' id='postlist'>
		<tr><th class='no'>���</th><th class='mark'>���</th><th>����</th><th class='time'>����ʱ��</th><th class='ptitle'>����</th></tr>
		<xsl:for-each select='po'><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:choose><xsl:when test='@sticky'>���ޡ�</xsl:when><xsl:otherwise><xsl:value-of select='position() - 1 + ../brd/@start'/></xsl:otherwise></xsl:choose></td>
			<td class='mark'><xsl:value-of select='@m'/></td>
			<td class='owner'><a class='owner' href='qry?u={@owner}'><xsl:value-of select='@owner'/></a></td>
			<td class='time'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@time'/></xsl:call-template></td>
			<td class='ptitle'><a class='ptitle'>
				<xsl:attribute name='href'><xsl:value-of select='../brd/@link'/>con?new=1&amp;bid=<xsl:value-of select='../brd/@bid'/>&amp;f=<xsl:value-of select='@id'/><xsl:if test='@sticky'>&amp;s=1</xsl:if></xsl:attribute>
				<xsl:if test='substring(., 1, 4) != "Re: "'><img src='../images/types/text.gif'/></xsl:if>
				<xsl:call-template name='ansi-escape'>
					<xsl:with-param name='content'><xsl:value-of select='.'/></xsl:with-param>
					<xsl:with-param name='fgcolor'>37</xsl:with-param>
					<xsl:with-param name='bgcolor'>ignore</xsl:with-param>
					<xsl:with-param name='ishl'>0</xsl:with-param>
				</xsl:call-template>
			</a></td>
		</tr></xsl:for-each>
	</table>

	<div class='blink'>
		<xsl:if test='brd/@link != ""'><a href='doc?bid={brd/@bid}'><img src='../images/button/home.gif'/>һ��ģʽ</a></xsl:if>
		<xsl:if test='brd/@link != "t"'><a href='tdoc?bid={brd/@bid}'><img src='../images/button/content.gif'/>����ģʽ</a></xsl:if>
		<xsl:if test='brd/@link != "g"'><a href='gdoc?bid={brd/@bid}'>��ժ��</a></xsl:if>
		<a href='0an?bid={brd/@bid}'><img src='../images/announce.gif'/>������</a>
		<a href='bfind?bid={brd/@bid}'><img src='../images/search.gif'/>��������</a>
		<a href='rss?bid={brd/@bid}'>RSS</a>
	</div>
</xsl:template>

<xsl:template match='bbscon'>
	<div class='post'>
		<div class='ptop'>
			<xsl:if test='@link != "con"'><a href='gdoc?bid={@bid}'>��ժ��</a></xsl:if>
			<a href='doc?bid={@bid}'><img src='../images/button/home.gif'/>��������</a>
			<xsl:variable name='baseurl'>con?new=1&amp;bid=<xsl:value-of select='@bid'/>&amp;f=<xsl:value-of select='po/@fid'/>&amp;a=</xsl:variable>
			<xsl:if test='not(po/@sticky)'>
				<xsl:if test='not(po/@first)'><a href='{$baseurl}p'><img src='../images/button/up.gif'/>��ƪ</a></xsl:if>
				<xsl:if test='not(po/@last)'><a href='{$baseurl}n'><img src='../images/button/down.gif'/>��ƪ</a></xsl:if>
				<xsl:if test='po/@reid != f'><a href='{$baseurl}b'>��¥</a></xsl:if>
				<xsl:if test='not(po/@tlast)'><a href='{$baseurl}a'>��¥</a></xsl:if>
				<xsl:if test='po/@gid'><a href='con?new=1&amp;bid={@bid}&amp;f={po/@gid}'>��¥</a></xsl:if>
				<xsl:variable name='gid'><xsl:choose><xsl:when test='po/@gid'><xsl:value-of select='po/@gid'/></xsl:when><xsl:otherwise><xsl:value-of select='po/@fid'/></xsl:otherwise></xsl:choose></xsl:variable>
				<xsl:if test='po/@fid != po/@gid or not(po/@tlast)'><a href='tcon?new=1&amp;bid={@bid}&amp;f={$gid}'>չ������</a></xsl:if>
				<xsl:if test='not(po/@tlast)'><a href='tcon?new=1&amp;bid={@bid}&amp;g={$gid}&amp;f={po/@fid}&amp;a=n'>���չ��</a></xsl:if>
			</xsl:if>
			<a><xsl:attribute name='href'>con?new=1&amp;bid=<xsl:value-of select='@bid'/>&amp;f=<xsl:value-of select='po/@fid'/><xsl:if test='po/@sticky'>&amp;s=1</xsl:if></xsl:attribute>��������</a>
			<a><xsl:attribute name='href'>../static/con?bid=<xsl:value-of select='@bid'/>&amp;f=<xsl:value-of select='po/@fid'/><xsl:if test='po/@sticky'>&amp;s=1</xsl:if></xsl:attribute>����/��ӡ</a>
			<xsl:call-template name='sigature-options'/>
		</div>

		<div class='pmain'><xsl:apply-templates select='po'/></div>
		<div class='plink'><xsl:call-template name='con-linkbar'/></div>
	</div>
	<xsl:call-template name='quick-reply-form'/>
</xsl:template>

<xsl:template name='quick-reply-form'>
<form id='quick_reply' class='quick_reply' method='post'>
<div class='buttons'>
<input type='button' class='cancel' value='ȡ��'/>
<xsl:if test='@anony=1'><input type="checkbox" name="anony" value="1" checked="checked"/>����</xsl:if>
ǩ���� <select name='sig'><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option><option value="5">5</option><option value="6">6</option></select>
<input type='button' value='����' class='confirm'/><img src='../images/indicator.gif' class='loading'/>
</div>
<div>
<div>���� <input class='binput' type='text' name='title' size='60' maxlength='50'></input></div>
<textarea class='binput' name='text' rows='10' cols='85' wrap='virtual'></textarea>
</div>
</form>
</xsl:template>

<xsl:template match='po'>
<div class='post_h'>
	<p>������: <a class='powner' href='qry?u={owner}'><xsl:value-of select='owner'/></a> (<xsl:value-of select='nick'/>), ����: <a href='doc?board={board}'><xsl:value-of select='board'/></a></p>
	<p>��&#160;&#160;��: <span class='ptitle'><xsl:value-of select='title'/></span></p>
	<p>����վ: ����Ȫ (<xsl:value-of select='date'/>), վ���ż�</p>
</div>
<xsl:for-each select='pa'>
	<div class='post_{@m}'>
		<xsl:for-each select='p'><p><xsl:apply-templates select='.'/></p></xsl:for-each>
	</div>
</xsl:for-each>
</xsl:template>

<xsl:template match='br'><br/></xsl:template>

<xsl:template match='c'>
<span class='a{@h}{@f} a{@b}'><xsl:value-of select='.'/></span>
</xsl:template>

<xsl:template match='a'>
<a href='{@href}'><xsl:choose><xsl:when test='@i'><img src='{@href}'/></xsl:when><xsl:otherwise><xsl:value-of select='@href'/></xsl:otherwise></xsl:choose></a>
</xsl:template>

<xsl:template name='con-linkbar'>
	<xsl:variable name='param'>bid=<xsl:value-of select='@bid'/>&amp;f=<xsl:value-of select='po/@fid'/></xsl:variable>
	<xsl:if test='po/@nore'><span class='disabled'>���Ĳ��ɻظ�</span></xsl:if>
	<xsl:if test='not(po/@nore) and @link="con"'><a class='reply'><xsl:attribute name='href'>pst?<xsl:value-of select='$param'/></xsl:attribute>�ظ�����</a></xsl:if>
	<xsl:if test='po/@edit'>
		<a href='edit?{$param}'>�޸�</a>
		<a href='del?{$param}'>ɾ��</a>
	</xsl:if>
	<a href='ccc?{$param}'>ת��</a>
	<a href='fwd?{$param}'>ת��</a>
</xsl:template>

<xsl:template match='bbstcon'>
	<div class='pnav'>
		<xsl:call-template name='tcon-navbar'/>
		<xsl:call-template name='sigature-options'/>
	</div>
	<xsl:for-each select='po'>
		<div class='post'>
			<div class='pmain'><xsl:apply-templates select='.'/></div>
			<div class='plink'>
				<xsl:if test='@nore'><span class='disabled'>���Ĳ��ɻظ�</span></xsl:if>
				<xsl:if test='not(@nore)'><a class='reply' href='pst?bid={../@bid}&amp;f={@fid}'>�ظ�����</a></xsl:if>
				<a href='ccc?bid={../@bid}&amp;f={@fid}'>ת��</a>
				<a href='con?new=1&amp;bid={../@bid}&amp;f={@fid}'><img src='../images/button/content.gif'/>��������</a>
			</div>
		</div>
	</xsl:for-each>
	<div class='pnav'><xsl:call-template name='tcon-navbar'/></div>
	<xsl:call-template name='quick-reply-form'/>
</xsl:template>

<xsl:template name='tcon-navbar'>
		<a href='tdoc?bid={@bid}'><img src='../images/button/home.gif'/>��������</a>
		<xsl:if test='count(po) = @page'><a href='tcon?new=1&amp;bid={@bid}&amp;g={@gid}&amp;f={po[last()]/@fid}&amp;a=n'><img src='../images/button/down.gif'/>��ҳ</a></xsl:if>
		<xsl:if test='po[1]/@fid != @gid'><a href='tcon?new=1&amp;bid={@bid}&amp;g={@gid}&amp;f={po[1]/@fid}&amp;a=p'><img src='../images/button/up.gif'/>��ҳ</a></xsl:if>
		<xsl:if test='not(@tlast)'><a href='tcon?new=1&amp;bid={@bid}&amp;f={@gid}&amp;a=a'>��һ����</a></xsl:if>
		<xsl:if test='not(@tfirst)'><a href='tcon?new=1&amp;bid={@bid}&amp;f={@gid}&amp;a=b'>��һ����</a></xsl:if>
</xsl:template>

<xsl:template name='sigature-options'>
	<a href='#' class='sig_option'>ǩ����ѡ��</a>
	<form class='sig_option' action='sigopt'>
		<input type='checkbox' name='hidesig'><xsl:if test='@nosig'><xsl:attribute name='checked'>checked</xsl:attribute></xsl:if></input>����ǩ����<input type='checkbox' name='hideimg'><xsl:if test='@nosigimg'><xsl:attribute name='checked'>checked</xsl:attribute></xsl:if></input>����ǩ����ͼƬ<input type='submit' value='����'/><input type='button' class='cancel' value='ȡ��'/>
	</form>
</xsl:template>

<xsl:template match='bbsqry'>
	<form action='qry' method='get'><label for='u'>����������ѯ���ʺţ�</label><input type='text' name='u' maxlength='12' size='12'/><input type='submit' value='��ѯ'/></form>
	<xsl:choose><xsl:when test='@login'><div class='post'>
		<div class='ptop'><a href='pstmail?recv={@id}'>�����ż�</a></div>
		<div class='umain' id='uinfo'>
		<p><strong><xsl:value-of select='@id'/></strong> ��<strong><xsl:value-of select='nick'/></strong>�� <xsl:call-template name='show-horo'/></p>
		<p>�ϴ���:��<span class='a132'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@lastlogin'/></xsl:call-template></span>���ӡ�<span class='a132'><xsl:value-of select='ip'/></span>������վһ�Ρ�</p>
		<xsl:if test='logout'><p>��վ��:��<span class='a132'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='logout'/></xsl:call-template></span>��</p></xsl:if>
		<p>������:��<span class='a132'><xsl:value-of select='@post'/></span>�� ������:��<span class='a132'><xsl:value-of select='@hp'/></span>��</p> 
		<p>����ֵ:��<span class='a133'><xsl:value-of select='@perf'/></span>��</p>
		<p>����ֵ:��<xsl:call-template name="show-exp"/>�� (<xsl:value-of select='@level * 10 + @repeat'/>/60)</p>
		<p>���: <xsl:call-template name='ansi-escape'><xsl:with-param name='content'><xsl:value-of select='ident'/></xsl:with-param><xsl:with-param name='fgcolor'>37</xsl:with-param><xsl:with-param name='bgcolor'>ignore</xsl:with-param><xsl:with-param name='ishl'>0</xsl:with-param></xsl:call-template></p></div>
		<xsl:if test='st'><div class='usplit'>Ŀǰ״̬</div>
		<div class='umain'><xsl:for-each select='st'><p><strong><xsl:value-of select='@desc'/></strong><xsl:if test='@idle!=0'>[����<xsl:value-of select='@idle'/>����]</xsl:if><xsl:if test='@web=1'>��web��¼��</xsl:if><xsl:if test='@vis=0'>������</xsl:if></p></xsl:for-each></div></xsl:if>
		<div class='usplit'>����˵��������</div>
		<div class='usmd'><xsl:call-template name='showpost'><xsl:with-param name='content' select='smd'/></xsl:call-template></div>
	</div></xsl:when><xsl:otherwise><xsl:if test='@id!=""'><p>û���ҵ��û���<xsl:value-of select='@id'/>��</p></xsl:if></xsl:otherwise></xsl:choose>
</xsl:template>

<xsl:template name='show-horo'>
	<xsl:if test='@horo'>
		<xsl:variable name='color'><xsl:choose><xsl:when test='@gender = "M"'>a136</xsl:when><xsl:when test='@gender = "F"'>a135</xsl:when><xsl:otherwise>a132</xsl:otherwise></xsl:choose></xsl:variable>
		<span>��</span><span class='{$color}'><xsl:value-of select='@horo'/></span><span>��</span>
	</xsl:if>
</xsl:template>

<xsl:template name='show-exp'>
	<span class='lev{@level}'>
		<span class='lev{@level}' style='width:{@repeat * 10}%;'></span>
	</span>
</xsl:template>

<xsl:template match='bbspst'>
	<p>���棺<xsl:value-of select='@brd'/></p>
	<form id='postform' name='postform' method='post' action='snd?bid={@bid}&amp;f={po/@f}&amp;e={@edit}'>
		<input type='hidden' id='brd' value='{@brd}'></input>
		<p>���⣺<xsl:choose>
		<xsl:when test='@edit=0'><input class='binput' type='text' name='title' size='60' maxlength='50'>
			<xsl:variable name='retitle'>
				<xsl:choose>
					<xsl:when test='substring(t, 1, 4) = "Re: "'><xsl:value-of select='t'/></xsl:when>
					<xsl:when test='not(t)'></xsl:when>
					<xsl:otherwise><xsl:value-of select='concat("Re: ", t)'/></xsl:otherwise>
				</xsl:choose>
			</xsl:variable>
			<xsl:attribute name='value'>
				<xsl:call-template name='remove-ansi'>
					<xsl:with-param name='str' select='$retitle'/>
				</xsl:call-template>
			</xsl:attribute>
		</input></xsl:when>
		<xsl:otherwise><xsl:call-template name='remove-ansi'><xsl:with-param name='str' select='t'/></xsl:call-template></xsl:otherwise>
		</xsl:choose></p>
		<p><xsl:if test='@anony=1'><input type="checkbox" name="anony" value="1" checked="checked"/>���� </xsl:if>ǩ����: <input type='radio' name='sig' value='1' checked='checked'/>1 <input type='radio' name='sig' value='2'/>2 <input type='radio' name='sig' value='3'/>3 <input type='radio' name='sig' value='4'/>4 <input type='radio' name='sig' value='5'/>5 <input type='radio' name='sig' value='6'/>6</p>
		<p><textarea class='binput' name='text' rows='20' cols='85' wrap='virtual'>
			<xsl:if test='@edit=0'><xsl:text> &#x0d;&#x0a;</xsl:text></xsl:if>
			<xsl:call-template name='show-quoted'>
				<xsl:with-param name='content' select='po'/>
			</xsl:call-template>
		</textarea></p>
		<input type='submit' value='����' id='btnPost' size='10'/>
		<input type='reset' value='��ԭ' size='10'/>
		<xsl:if test='@edit="0" and @att!=0'><input type='button' name='attach' value='�ϴ�����' onclick='return preUpload() '/></xsl:if>
	</form>
	<xsl:choose>
		<xsl:when test='not(t)'><script type='text/javascript' defer='defer'>addLoadEvent(function(){document.postform.title.focus();})</script></xsl:when>
		<xsl:otherwise><script type='text/javascript' defer='defer'>addLoadEvent(function() {var text = document.postform.text; text.selectionStart = 0; text.selectionEnd = 1; text.focus();})</script></xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='bbstop10'>
	<h2>24Сʱʮ�����Ż���</h2>
	<table class='content'>
		<tr><th class='no'>����</th><th class='owner'>����</th><th class='title'>����</th><th class='no'>ƪ��</th><th class='ptitle'>����</th></tr>
		<xsl:for-each select='top'><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:value-of select='position()'/></td>
			<td class='owner'><a class='owner' href='qry?u={@owner}'><xsl:value-of select='@owner'/></a></td>
			<td class='title'><a class='title' href='doc?board={@board}'><xsl:value-of select='@board'/></a></td>
			<td class='no'><xsl:value-of select='@count'/></td>
			<td class='ptitle'><a class='ptitle' href='tcon?new=1&amp;board={@board}&amp;f={@gid}'><xsl:call-template name='ansi-escape'><xsl:with-param name='content' select='.'/><xsl:with-param name='fgcolor'>37</xsl:with-param><xsl:with-param name='bgcolor'>ignore</xsl:with-param><xsl:with-param name='ishl'>0</xsl:with-param></xsl:call-template></a></td>
		</tr></xsl:for-each>
	</table>
</xsl:template>

<xsl:template match='bbsbfind'>
	<h2>������������</h2>
	<xsl:variable name='count' select='count(po)'/>
	<xsl:choose><xsl:when test='@result'><p>���ҵ� <xsl:value-of select='$count'/> ƪ���� <xsl:if test='$count&gt;=100'>��100ƪ���ϲ���ʡ�ԣ�</xsl:if></p><xsl:if test='$count!=0'><p>�������¿�ǰ</p><table class='content'>
		<tr><th class='no'>���</th><th class='mark'>���</th><th class='owner'>����</th><th class='time'>����ʱ��</th><th class='ptitle'>����</th></tr>
		<xsl:for-each select='po'><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:value-of select='position()'/></td>
			<td class='mark'><xsl:value-of select='@m'/></td>
			<td class='owner'><a class='owner' href='qry?u={@owner}'><xsl:value-of select='@owner'/></a></td>
			<td class='time'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@time'/></xsl:call-template></td>
			<xsl:variable name='imgsrc'>../images/types/<xsl:choose><xsl:when test='substring(., 1, 4) = "Re: "'>reply</xsl:when><xsl:otherwise>text</xsl:otherwise></xsl:choose>.gif</xsl:variable>
			<xsl:variable name='text'><xsl:choose><xsl:when test='substring(., 1, 4) = "Re: "'><xsl:value-of select='substring(., 5)'/></xsl:when><xsl:otherwise><xsl:value-of select='.'/></xsl:otherwise></xsl:choose></xsl:variable>
			<td class='ptitle'><a class='ptitle' href='{../brd/@link}con?new=1&amp;bid={../@bid}&amp;f={@id}'>
				<img src='{$imgsrc}'/>
				<xsl:call-template name='ansi-escape'>
					<xsl:with-param name='content'><xsl:value-of select='$text'/></xsl:with-param>
					<xsl:with-param name='fgcolor'>37</xsl:with-param>
					<xsl:with-param name='bgcolor'>ignore</xsl:with-param>
					<xsl:with-param name='ishl'>0</xsl:with-param>
				</xsl:call-template>
			</a></td>
		</tr></xsl:for-each>
	</table></xsl:if></xsl:when>
	<xsl:otherwise><form action='bfind' method='get'><fieldset><legend>������дһ��</legend>
		<input name='bid' type='hidden' value='{@bid}'></input>
		<p>���⺬��: <input name='t1' type='text' maxlength='50' size='20'/> �� <input name='t2' type='text' maxlength='50' size='20'/></p>
		<p>���ⲻ��: <input name='t3' type='text' maxlength='50' size='20'/></p>
		<p>�����ʺ�: <input name='user' type='text' maxlength='12' size='16'/></p></fieldset>
		<fieldset><legend>ѡ��</legend><p>ʱ�䷶Χ: <input name='limit' type='text' maxlength='4' size='4' value='7'/>������ (����30��)</p>
		<p>���±��: <input name='mark' type='checkbox'/></p>
		<p>��������: <input name='nore' type='checkbox'/></p></fieldset>
		<p><input type='submit' value='�����ѣ�'/></p>
	</form></xsl:otherwise></xsl:choose>
</xsl:template>

<xsl:template match='bbsnewmail'>
	<h2>�������ż�</h2>
	<p><a href='mail'><xsl:choose><xsl:when test='count(mail)=0'>��û��30���ڵ�δ���ż�</xsl:when><xsl:otherwise>��ҳ����ʾ30����δ���ż�</xsl:otherwise></xsl:choose>���鿴ȫ���ż����˴�</a></p>
	<xsl:if test='count(mail)!=0'><form name='list' method='post' action='mailman'>
		<table class='content'>
			<tr><th class='no'>���</th><th class='chkbox'>����</th><th class='owner'>������</th><th class='time'>����</th><th class='ptitle'>�ż�����</th></tr>
			<xsl:for-each select='mail'><tr>
				<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
				<td class='no'><xsl:value-of select='@n'/></td>
				<td class='chkbox'><input type='checkbox' name='box{@name}'></input></td>
				<td class='owner'><a class='owner' href='qry?u={@from}'><xsl:value-of select='@from'/></a></td>
				<td class='time'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@date'/></xsl:call-template></td>
				<td class='ptitle'><a class='ptitle' href='mailcon?f={@name}&amp;n={@n}'>
					<xsl:call-template name='ansi-escape'>
						<xsl:with-param name='content'><xsl:value-of select='.'/></xsl:with-param>
						<xsl:with-param name='fgcolor'>37</xsl:with-param>
						<xsl:with-param name='bgcolor'>ignore</xsl:with-param>
						<xsl:with-param name='ishl'>0</xsl:with-param>
					</xsl:call-template>
				</a></td>
			</tr></xsl:for-each>
		</table>
		<input name='mode' value='' type='hidden'/>
	</form>
	<div>[<a href="#"  onclick="checkAll();">ȫѡ</a>] [<a href="#"  onclick="checkReverse();">��ѡ</a>] [<a href="#" onclick="delSelected()">ɾ����ѡ�ż�</a>]</div></xsl:if>
</xsl:template>

<xsl:template match='bbsmail'>
	<h2>�ż��б�</h2>
	<p>�ż����� [<xsl:value-of select='@total'/>]��</p>
	<form name='list' method='post' action='mailman'>
		<table class='content'>
			<tr><th class='no'>���</th><th class='chkbox'>����</th><th class='mark'>״̬</th><th class='owner'>������</th><th class='time'>����</th><th class='ptitle'>�ż�����</th></tr>
			<xsl:for-each select='mail'><tr>
				<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
				<td class='no'><xsl:value-of select='position() - 1 + ../@start'/></td>
				<td class='chkbox'><input type="checkbox" name='box{@name}'></input></td>
				<td class='mark'><xsl:value-of select='@m'/></td>
				<td><a class='owner' href='qry?u={@from}'><xsl:value-of select='@from'/></a></td>
				<td class='time'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@date'/></xsl:call-template></td>
				<td class='ptitle'><a class='ptitle' href='mailcon?f={@name}&amp;n={position()-1+../@start}'>
					<xsl:call-template name='ansi-escape'>
						<xsl:with-param name='content'><xsl:value-of select='.'/></xsl:with-param>
						<xsl:with-param name='fgcolor'>37</xsl:with-param>
						<xsl:with-param name='bgcolor'>ignore</xsl:with-param>
						<xsl:with-param name='ishl'>0</xsl:with-param>
					</xsl:call-template>
				</a></td>
			</tr></xsl:for-each>
		</table>
		<input name='mode' value='' type='hidden'/>
	</form>
	<div>[<a href="#"  onclick="checkAll();">��ҳȫѡ</a>] [<a href="#"  onclick="checkReverse();">��ҳ��ѡ</a>] [<a href="#" onclick="delSelected()">ɾ����ѡ�ż�</a>]</div>
	<div>
		<xsl:if test='@start &gt; 1'>
			<xsl:variable name='prev'>
				<xsl:choose><xsl:when test='@start - @page &lt; 1'>1</xsl:when><xsl:otherwise><xsl:value-of select='@start - @page'/></xsl:otherwise></xsl:choose>
			</xsl:variable>
			<a href='mail?start={$prev}'>[ <img src='../images/button/up.gif'/>��һҳ ]</a>
		</xsl:if>
		<xsl:if test='@total &gt; @start + @page - 1'>
			<xsl:variable name='next'><xsl:value-of select='@start + @page'/></xsl:variable>
			<a href='mail?start={$next}'>[ <img src='../images/button/down.gif'/>��һҳ ]</a>
		</xsl:if>
		<form><input value='��ת��' type='submit'/>��<input name='start' size='4' type='text'/>��</form>
	</div>
</xsl:template>

<xsl:template match='bbsmailcon'>
	<div class='post'>
		<div class='ptop'><xsl:call-template name='mailcon-navbar'/></div>
		<div class='plink'><xsl:call-template name='mailcon-linkbar'/></div>
		<div class='pmain'><xsl:call-template name='showpost'><xsl:with-param name='content' select='mail'/></xsl:call-template></div>
		<div class='plink'><xsl:call-template name='mailcon-linkbar'/></div>
		<div class='pbot'><xsl:call-template name='mailcon-navbar'/></div>
	</div>
	<xsl:if test='@new'><script type='text/javascript' defer='defer'>bbs.init();bbs.store.set('last', 0);</script></xsl:if>
</xsl:template>

<xsl:template name='mailcon-navbar'>
	<a href='mail'>[ <img src='../images/button/back.gif'/>�ż��б� ]</a>
	<xsl:if test='@prev'><a href='mailcon?f={@prev}&amp;n={mail/@n-1}'>[ <img src='../images/button/up.gif'/>��һ�� ]</a></xsl:if>
	<xsl:if test='@next'><a href='mailcon?f={@next}&amp;n={mail/@n+1}'>[ <img src='../images/button/down.gif'/>��һ�� ]</a></xsl:if>
</xsl:template>

<xsl:template name='mailcon-linkbar'>
	<a href='pstmail?n={mail/@n}'>[ <img src='../images/button/edit.gif'/>�ظ����� ]</a>
	<a onclick='return confirm("�����Ҫɾ���������")' href='delmail?f={mail/@f}'>[ ɾ������ ]</a>
</xsl:template>

<xsl:template match='bbspstmail'>
	<form id='postform' name='postform' method='post' action='sndmail'>
		<input type='hidden' name='ref' value='{@ref}'></input>
		<p><label for='recv'>������:&#160;&#160;&#160;</label><input class='binput' type='text' name='recv' size='15' maxlength='15' value='{@recv}'></input></p>
		<p><label for='title'>�ż����� </label>
		<input class='binput' type='text' name='title' size='60' maxlength='50'>
			<xsl:variable name='retitle'>
				<xsl:choose>
					<xsl:when test='substring(t, 1, 4) = "Re: "'><xsl:value-of select='t'/></xsl:when>
					<xsl:when test='not(t)'></xsl:when>
					<xsl:otherwise><xsl:value-of select='concat("Re: ", t)'/></xsl:otherwise>
				</xsl:choose>
			</xsl:variable>
			<xsl:attribute name='value'>
				<xsl:call-template name='remove-ansi'>
					<xsl:with-param name='str' select='$retitle'/>
				</xsl:call-template>
			</xsl:attribute>
		</input></p>
		<p>���ݸ��Լ� <input type='checkbox' name='backup' value='backup'/></p>
		<p><textarea class='binput' name='text' rows='20' cols='85' wrap='virtual'>
			<xsl:text>&#x0d;&#x0a;</xsl:text>
			<xsl:call-template name='show-quoted'>
				<xsl:with-param name='content' select='m'/>
			</xsl:call-template>
		</textarea></p>
		<input type='submit' value='�ĳ�' id='btnPost' size='10'/>
		<input type='reset' value='����'/>
	</form>
</xsl:template>

<xsl:template match='bbs0an'>
	<p>��Ŀ¼web���������[<xsl:value-of select='@v'/>]</p>
	<table class='content'>
		<tr><th class='no'>���</th><th class='ptitle'>����</th><th class='bm'>������</th><th class='time'>����</th></tr>
		<xsl:for-each select='ent'><tr>
				<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
				<td class='no'><xsl:value-of select='position()'/></td>
				<td class='ptitle'><xsl:choose>
					<xsl:when test='@t = "d"'><a class='ptitle' href='0an?path={../@path}{@path}'><img src='../images/types/folder.gif'/><xsl:value-of select='.'/></a></xsl:when>
					<xsl:when test='@t = "f"'><a class='ptitle' href='anc?path={../@path}{@path}'><img src='../images/types/text.gif'/><xsl:value-of select='.'/></a></xsl:when>
					<xsl:otherwise><img src='../images/types/error.gif'/><xsl:value-of select='@t'/></xsl:otherwise>
				</xsl:choose></td>
				<td class='bm'><xsl:if test='@id'>
					<xsl:call-template name='splitbm'>
						<xsl:with-param name='names' select='@id'/>
						<xsl:with-param name='isdir' select='0'/>
						<xsl:with-param name='isfirst' select='1'/>
					</xsl:call-template>
				</xsl:if></td>
				<td class='time'><xsl:if test='@t != "e"'><xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@time'/></xsl:call-template></xsl:if></td>
		</tr></xsl:for-each>
		<xsl:if test='not(ent)'>
			<td/><td width='80%'>&lt;&lt;Ŀǰû������&gt;&gt;</td>
		</xsl:if>
	</table>
	<xsl:if test='@brd'><a href='doc?board={@brd}'>[<img src='../images/button/home.gif'/>��������]</a></xsl:if>
</xsl:template>

<xsl:template match='bbsanc'>
	<h3><xsl:if test='@brd'><xsl:value-of select='@brd'/>�� - </xsl:if>�����������Ķ�</h3>
	<div class='post'>
		<div class='ptop'><xsl:call-template name='anc-navbar'/></div>
		<div class='pmain'><xsl:call-template name='showpost'><xsl:with-param name='content' select='po'/></xsl:call-template></div>
		<div class='pbot'><xsl:call-template name='anc-navbar'/></div>
	</div>
</xsl:template>

<xsl:template name='anc-navbar'>
	<xsl:if test='@brd'>
		<a href='gdoc?board={@brd}'>[ ��ժ�� ]</a>
		<a href='doc?board={@brd}'>[<img src='../images/button/home.gif'/>��������]</a>
	</xsl:if>
</xsl:template>

<xsl:template match='bbsfwd'>
	<form action='fwd' method='post'>
		<input type='hidden' name='bid' value='{@bid}'></input>
		<input type='hidden' name='f' value='{@f}'></input>
		<label for='u'>������:&#160;</label><input type='text' name='u' size='16'></input><br/>
		<input value='ת��' type='submit'/>
	</form>
</xsl:template>

<xsl:template match='bbsccc'>
	<xsl:choose>
		<xsl:when test='not(@bid)'>
			<p>ת�سɹ�</p>
			<p><a href='doc?bid={@b}'>[ <img src='../images/button/back.gif'/>����ԭ�Ȱ��� ]</a></p>
			<p><a href='doc?bid={@t}'>[ <img src='../images/button/forward.gif'/>����Ŀ����� ]</a></p>
		</xsl:when>
		<xsl:otherwise>
			<form method='get' action='ccc'>
				<p>���±���: <xsl:value-of select='node()[1]'/></p>
				<p>��������: <xsl:value-of select='@owner'/></p>
				<p>ԭʼ����: <xsl:value-of select='@brd'/></p>
				<input type='hidden' name='bid' value='{@bid}'></input>
				<input type='hidden' name='f' value='{@fid}'></input>
				<label for='t'>ת�ص�����: </label><input type='text' name='t'/>
				<input type='submit' value='ת��'/>
				<p><strong>ת��ע�⣺δ��վ��ίԱ����׼�������ת����ͬ���������³��������ģ����ܵ�ȫվ������</strong></p>
			</form>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='bbsfall'>
	<h2>�趨��ע����</h2>
	<table class='content'>
		<tr><th class='owner'>�ʺ�</th><th class='chkbox'>����</th><th class='idesc'>˵��</th></tr>
		<xsl:for-each select='ov'><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='owner'><a class='owner' href='qry?u={@id}'><xsl:value-of select='@id'/></a></td>
			<td class='chkbox'><a href='fdel?u={@id}'>ɾ��</a></td>
			<td class='idesc'><xsl:value-of select='.'/></td>
		</tr></xsl:for-each>
	</table>
	<a href='fadd'>[���ӹ�ע����]</a>
</xsl:template>

<xsl:template match='bbsfadd'>
	<h2>���ӹ�ע����</h2>
	<form name='add' method='get' action='fadd'>
		<p><label for='id'>�ʺ�: </label><input class='binput' type='text' name='id' size='15' maxlength='15'></input></p>
		<p><label for='id'>˵��: </label><input class='binput' type='text' name='desc' size='50' maxlength='50'></input></p>
		<p><input type='submit' value='�ύ' size='10'/></p>
	</form>
</xsl:template>

<xsl:template match='bbsovr'>
	<table class='content'>
		<tr><th class='no'>���</th><th class='owner'>ʹ���ߴ���</th><th class='idesc'>�ǳ�</th><th>��վλ��</th><th>��̬</th><th>����</th></tr>
		<xsl:for-each select='ov'><xsl:sort select='@id'/><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:value-of select='position()'/></td>
			<td class='owner'><a class='owner' href='qry?u={@id}'><xsl:value-of select='@id'/></a></td>
			<td class='idesc'><xsl:value-of select='.'/></td>
			<td><xsl:value-of select='@ip'/></td>
			<td><xsl:value-of select='@action'/></td>
			<td><xsl:if test='@idle &gt; 0'><xsl:value-of select='@idle'/></xsl:if></td>
		</tr></xsl:for-each>
	</table>
</xsl:template>

<xsl:template match='bbsbrdadd'>
	<h2>����ղذ���</h2>
	<p>�ɹ���� <a href='doc?bid={bid}'><xsl:value-of select='brd'/></a> �浽�ղؼ�</p>
</xsl:template>

<xsl:template match='bbsfav'>
	<h2>�ҵ��ղؼ�</h2>
	<p><a href='mybrd'>�Զ���</a></p>
	<table class='content'>
		<tr><th class='no'>���</th><th class='title'>����������</th><th class='desc'>��������</th></tr>
		<xsl:for-each select='brd'><tr>
			<xsl:attribute name='class'><xsl:choose><xsl:when test='position() mod 2 = 1'>light</xsl:when><xsl:otherwise>dark</xsl:otherwise></xsl:choose></xsl:attribute>
			<td class='no'><xsl:value-of select='position()'/></td>
			<td class='title'><a class='title' href='doc?bid={@bid}'><xsl:value-of select='@brd'/></a></td>
			<td class='desc'><a class='desc' href='doc?bid={@bid}'><xsl:value-of select='.'/></a></td>
		</tr></xsl:for-each>
	</table>
</xsl:template>

<xsl:template match='bbsmybrd'>
	<h2>�ղؼ��趨</h2>
	<xsl:choose>
		<xsl:when test='@selected'>
			<div>�޸�Ԥ���������ɹ���������һ��Ԥ���� <xsl:value-of select='@selected'/> ��������</div>
		</xsl:when>
		<xsl:otherwise>
			<form action='mybrd?type=1' method='post'>
				<div class='column-3'><ul>
					<xsl:apply-templates select='mbrd'/>
				</ul></div>
				<input type='submit' value='ȷ��Ԥ��'/><input type='reset' value='��ԭ'/>
			</form>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='mbrd'>
	<xsl:variable name='check'><xsl:call-template name='is-mybrd'><xsl:with-param name='bid' select='@bid'/></xsl:call-template></xsl:variable>
	<li>
		<input type='checkbox' name='{@bid}'>
			<xsl:if test='$check = 1'><xsl:attribute name='checked'>checked</xsl:attribute></xsl:if>
		</input>
		<a class='idesc' href='doc?bid={@bid}'><xsl:value-of select='@name'/> - <xsl:value-of select='@desc'/></a>
	</li>
</xsl:template>

<xsl:template name='is-mybrd'>
	<xsl:param name='bid'/>
	<xsl:for-each select='../my'><xsl:if test='@bid = $bid'>1</xsl:if></xsl:for-each>
</xsl:template>

<xsl:template match='bbseufile'>
	<h2><xsl:value-of select='@desc'/></h2>
	<xsl:choose>
		<xsl:when test='@submit'>
			<form name='postform' method='post' action='{@submit}'>
				<p><textarea class='binput' name='text' rows='20' cols='85' wrap='virtual'><xsl:call-template name='show-quoted'><xsl:with-param name='content' select='text'/></xsl:call-template></textarea></p>
				<p><input type='submit' value='����' id='btnPost' size='10'/></p>
			</form>
		</xsl:when>
		<xsl:otherwise><p>����ɹ�</p><a href='javascript:history.go(-2)'>����</a></xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='bbsinfo'>
	<xsl:choose><xsl:when test='@gender'>
		<fieldset><legend>�޸ĸ�������</legend><form action='info?type=1' method='post'>
			<p>�����ǳ�: <input type='text' name='nick' maxlength='30' value='{nick}'></input></p>
			<p>��������: <input type='text' name='year' size='4' maxlength='4' value='{@year+1900}'></input> �� <input type='text' name='month' size='2' maxlength='2' value='{@month}'></input> �� <input type='text' name='day' size='2' maxlength='2' value='{@day}'></input> ��</p>
			<p>�û��Ա�: <input type='radio' value='M' name='gender'><xsl:if test='@gender = "M"'><xsl:attribute name='checked'>checked</xsl:attribute></xsl:if></input> �� <input type='radio' value='F' name='gender'><xsl:if test='@gender = "F"'><xsl:attribute name='checked'>checked</xsl:attribute></xsl:if></input> Ů</p>
			<input type='submit' value='ȷ��'/> <input type='reset' value='��ԭ'/>
		</form></fieldset>
		<p>��¼��վ: <xsl:value-of select='@login'/> ��</p>
		<p>��վʱ��: <xsl:value-of select='floor(@stay div 60)'/> Сʱ <xsl:value-of select='@stay mod 60'/> ����</p>
		<p>�������: <xsl:value-of select='@post'/> ƪ</p>
		<p>�ʺŽ���: <xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@since'/></xsl:call-template></p>
		<p>�������: <xsl:call-template name='timeconvert'><xsl:with-param name='time' select='@last'/></xsl:call-template></p>
		<p>��Դ��ַ: <xsl:value-of select='@host'/></p>
	</xsl:when>
	<xsl:otherwise><xsl:choose><xsl:when test='string-length(.) = 0'>�޸ĸ������ϳɹ�<br/><a href='javascript:history.go(-2)'>����</a></xsl:when><xsl:otherwise><xsl:value-of select='.'/></xsl:otherwise></xsl:choose></xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='bbspwd'>
	<xsl:choose>
		<xsl:when test='@i'><form action='pwd' method='post'>
			<p><label for='pw1'>���ľ�����: </label><input maxlength='12' size='12' type='password' name='pw1'/></p>
			<p><label for='pw2'>����������: </label><input maxlength='12' size='12' type='password' name='pw2'/></p>
			<p><label for='pw3'>ȷ��������: </label><input maxlength='12' size='12' type='password' name='pw3'/></p>
			<input type='submit' value='ȷ���޸�'/>
		</form></xsl:when>
		<xsl:otherwise>
			<xsl:choose><xsl:when test='string-length(.)=0'>�޸�����ɹ�<br/><a href='javascript:history.go(-2)'>����</a></xsl:when><xsl:otherwise><xsl:value-of select='.'/><br/><a href='javascript:history.go(-1)'>����</a></xsl:otherwise></xsl:choose>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match='bbsnot'>
	<h2>���滭�� - [<xsl:value-of select='@brd'/>]</h2>
	<div class='post'>
		<div class='ptop'>
			<xsl:if test='@brd'>
				<a href='gdoc?board={@brd}'>��ժ��</a>
				<a href='doc?board={@brd}'><img src='../images/button/home.gif'/>��������</a>
			</xsl:if>
		</div>
		<div class='usmd'><xsl:call-template name='showpost'><xsl:with-param name='content' select='node()[1]'/></xsl:call-template></div>
	</div>
</xsl:template>

<xsl:template name='not-navbar'>
</xsl:template>

<xsl:template match='bbsreg'>
	<xsl:if test='@error=1'>
		<p>ע���ʺų���<xsl:value-of select='.'/></p>
		<p><a href='javascript:history.go(-1)'>���ٷ���</a></p>
	</xsl:if>
	<xsl:if test='@error=0'>
		<p>ע��ɹ���</p>
		<p><a href='http://mail.fudan.edu.cn/'>����˴���¼�������䣬���ռ�������</a></p>
	</xsl:if>
</xsl:template>

<xsl:template match='bbsactivate'>
	<xsl:if test='@success=1'>
		<p>�ʺųɹ����</p>
		<p><a href='login?next=sec'>���ڵ�¼</a></p>
	</xsl:if>
	<xsl:if test='@success=0'>
		<p>�ʺż���ʧ�� :( ���鼤������</p>
	</xsl:if>
</xsl:template>

</xsl:stylesheet>
