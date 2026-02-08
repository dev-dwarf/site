<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" 
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:atom="http://www.w3.org/2005/Atom"
                >
  <xsl:output method="html" encoding="utf-8" indent="yes" doctype-system="about:legacy-compat" />
  <xsl:template match="/">

  $

  <p><div class="center"> <img src='/assets/dd.png' /></div></p>
  <h2 id='center'>
   Logan Forman <a href='https://www.twitter.com/dev_dwarf'>@dev dwarf</a>
  </h2>
  <table>
    <tr>
      <th>Date</th>
      <th>Title</th>
      <th style="width: 50%;">Description</th>
    </tr>
    <xsl:for-each select="/rss/channel/item">
      <tr>
        <td><xsl:value-of select="substring(pubDate, 6, 11)" /></td>
        <td><a href="{link}"><xsl:value-of select="title" /></a></td>
        <td><xsl:value-of select="description" /></td>
      </tr>
    </xsl:for-each>
  </table>
  <p class="centert">
  <em>This page is an rss feed. Copy the URL into your reader to subscribe. Learn more at <a href="https://aboutfeeds.com/">About Feeds</a>.</em>
  </p>

  $

  </xsl:template>
</xsl:stylesheet>