<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" 
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" encoding="utf-8" indent="yes"/>

  <xsl:template match="/">
    <html lang="en">
      <head>
        <meta charset="utf-8"/>
        <meta name="viewport" content="width=device-width, initial-scale=1"/>
        <title> 0A posts </title>
        <link rel="stylesheet" href="/style.css" type="text/css"/>
      </head>
      <body>
        <script>
        var theme = localStorage.getItem('theme') || 'light'
        document.querySelector('body').setAttribute('data-theme', theme)
        function toggleNight() {
        console.log('toggle')
        theme = (theme == 'light')? 'night' : 'light'
        localStorage.setItem('theme', theme)
        document.querySelector('body').setAttribute('data-theme', theme);  
        }
        </script>
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
          <p>
          <em>This page is an rss feed. Copy the URL into your reader to subscribe. Learn more at <a href="https://aboutfeeds.com/">About Feeds</a>.</em>
          </p>
        </main>
        <nav>
        <hr />
        <table class='w33 left'><tr>
          <td><a href='/index.html'>home</a></td>
          <td><a href='/projects.html'>projects</a></td>
          <td><a style='text-decoration-color: #EE802F !important' href='/rss.xml'>
            posts</a></td>
          <td class='light'><a class='light' onClick='toggleNight()'>light</a></td>
          <td class='night'><a class='night' onClick='toggleNight()'>night</a></td>
        </tr></table>
        <table class='w33 right'><tr>
          <td><a href='https://github.com/dev-dwarf'>github</a></td>
          <td><a href='https://twitter.com/dev_dwarf'>twitter</a></td>
          <td><a href='https://store.steampowered.com/developer/dd'>steam</a></td>
          <td><a href='https://dev-dwarf.itch.io'>itch</a></td>
        </tr></table>
        <p><br/></p>
      </nav>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>