<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" 
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" encoding="utf-8" indent="yes"/>

  <xsl:template match="/">
    <html lang="en">
      <head>
        <meta charset="utf-8"/>
        <meta name="viewport" content="width=device-width, initial-scale=1"/>
        <title><xsl:value-of select="/rss/channel/title"/> — Feed</title>
        <link rel="stylesheet" href="/style.css" type="text/css"/>
      </head>
      <body>

        <main class='page-content' aria-label='Content'>
          <p>
          <div class="center"> <img src='/assets/dd.png' /> </div>
          </p>
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
        </main>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>