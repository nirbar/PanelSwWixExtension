<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:text="my-text"
                xmlns:wix="http://wixtoolset.org/schemas/v4/wxs"
                xmlns:msxsl="urn:schemas-microsoft-com:xslt" exclude-result-prefixes="msxsl">
  <xsl:output method="xml" indent="yes" encoding="utf-8" omit-xml-declaration="yes"/>

  <xsl:variable name="fileName">[FILE_NAME]</xsl:variable>

  <xsl:template match="@* | node()">
    <xsl:copy>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="//wix:File">
    <xsl:copy>
      <xsl:attribute name="Name">
        <xsl:value-of select="text:replaceAll( string($fileName), 'Name', 'NAME')"/>
      </xsl:attribute>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
  </xsl:template>

  <msxsl:script implements-prefix="text" language="JScript">
    function replaceAll(str, a, b) {
    return str.replace(RegExp(a, 'g'), b);
    }
  </msxsl:script>
</xsl:stylesheet>
