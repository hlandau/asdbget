<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <HTML>
      <HEAD>
	<CENTER><H1>AS/400 FILE DISPLAY</H1></CENTER>
      </HEAD>
      <BODY>
	<TABLE BGCOLOR="#6699CC" CELLPADDING="0" CELLSPACING="0" BORDER="1" WIDTH="100%">
	  <xsl:for-each select="ASDB/LAYOUT">
	  <TR>
	    <xsl:for-each select="FIELD">
	    <TD COLSPAN="2" BORDER="1">
		<xsl:value-of/>
	    </TD>	
	    </xsl:for-each>
 	  </TR>
	  </xsl:for-each>
	</TABLE>
	<P></P>
	<TABLE BGCOLOR="#6699CC" CELLPADDING="0" CELLSPACING="0" BORDER="1" WIDTH="100%">
        <xsl:for-each select="ASDB/RECORD">
	 <TR>
	   <xsl:for-each select="DATUM">
            <TD COLSPAN="2" BORDER="1">
               <xsl:value-of/>
            </TD>
	 </xsl:for-each>
         </TR> 
        </xsl:for-each>
	</TABLE>
      </BODY>
    </HTML>
  </xsl:template>
</xsl:stylesheet>
