import { mkdir, rm, writeFile } from 'node:fs/promises'
import path from 'node:path'

const baseUrl = 'https://im-mortal.cn'
const entryPath = '/%E5%87%A1%E4%BA%BA%E4%BF%AE%E4%BB%99%E4%BC%A0'
const entryUrl = `${baseUrl}${entryPath}`
const outDir = path.resolve('doc/design/im-mortal-cn-fanren')
const pagesDir = path.join(outDir, 'pages')
const markdownDir = path.join(outDir, 'markdown')
const assetsDir = path.join(outDir, 'assets')

function decodeEntities(input) {
  return input
    .replace(/&#x([0-9a-fA-F]+);/g, (_, hex) => String.fromCodePoint(parseInt(hex, 16)))
    .replace(/&#(\d+);/g, (_, dec) => String.fromCodePoint(parseInt(dec, 10)))
    .replace(/&nbsp;/g, ' ')
    .replace(/&amp;/g, '&')
    .replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>')
    .replace(/&quot;/g, '"')
    .replace(/&#39;/g, "'")
    .replace(/&times;/g, '×')
}

function cleanupInline(html) {
  return decodeEntities(
    html
      .replace(/<br\s*\/?>/gi, ' ')
      .replace(/<\/?(sup|sub)[^>]*>/gi, '')
      .replace(/<[^>]+>/g, ' ')
      .replace(/\s+/g, ' ')
      .trim(),
  )
}

function safeSlugFromPath(urlPath) {
  if (urlPath === entryPath) {
    return 'fanren_overview'
  }
  return urlPath.replace(/^\/+/, '').replace(/[^a-zA-Z0-9._-]+/g, '_')
}

function safeAssetName(srcUrl) {
  const url = new URL(srcUrl)
  const pathname = url.pathname.replace(/^\/+/, '')
  const suffix = path.extname(pathname) || '.bin'
  const stem = pathname.slice(0, pathname.length - suffix.length).replace(/[^a-zA-Z0-9._-]+/g, '_')
  return `${stem}${suffix}`
}

function extractTitle(html) {
  const titleMatch = html.match(/<title>([\s\S]*?)<\/title>/i)
  if (!titleMatch) {
    return '未命名页面'
  }
  return decodeEntities(titleMatch[1]).replace(/^\[\s*[^\]]+\s*\]\s*/, '').trim()
}

function extractFirstHeading(contentHtml) {
  const headingMatch = contentHtml.match(/<h[1-6][^>]*>([\s\S]*?)<\/h[1-6]>/i)
  return headingMatch ? cleanupInline(headingMatch[1]) : ''
}

function extractContent(html) {
  const contentMatch = html.match(/<!-- content --><div class="dw-content">([\s\S]*?)<\/div>\s*<!-- \/content -->/i)
  if (contentMatch) {
    return contentMatch[1]
  }
  const bodyMatch = html.match(/<body[\s\S]*?>([\s\S]*?)<\/body>/i)
  return bodyMatch ? bodyMatch[1] : html
}

function extractWikiLinks(html) {
  const matches = [...html.matchAll(/href="([^"]+)"/g)]
  const out = []
  for (const match of matches) {
    const href = match[1]
    if (!href.startsWith('/game_wiki/')) {
      continue
    }
    if (href.includes('?')) {
      continue
    }
    out.push(href)
  }
  return [...new Set(out)].sort()
}

function extractImageSources(contentHtml) {
  const matches = [...contentHtml.matchAll(/<img\b[^>]*src="([^"]+)"[^>]*>/gi)]
  return [...new Set(matches.map((match) => new URL(match[1], baseUrl).toString()))]
}

function htmlToMarkdown(contentHtml, assetMap) {
  let content = contentHtml

  content = content.replace(/<script[\s\S]*?<\/script>/gi, '')
  content = content.replace(/<style[\s\S]*?<\/style>/gi, '')
  content = content.replace(/<!--([\s\S]*?)-->/g, '')

  content = content.replace(/<img\b[^>]*>/gi, (tag) => {
    const srcMatch = tag.match(/\bsrc="([^"]+)"/i)
    const altMatch = tag.match(/\balt="([^"]*)"/i)
    const src = srcMatch ? new URL(srcMatch[1], baseUrl).toString() : ''
    const localName = assetMap.get(src)
    const alt = altMatch ? decodeEntities(altMatch[1]) : ''
    if (!localName) {
      return '\n'
    }
    const label = alt || localName
    return `\n![${label}](../assets/${localName})\n`
  })

  content = content.replace(/<h([1-6])[^>]*>([\s\S]*?)<\/h\1>/gi, (_, level, inner) => {
    const depth = Math.max(1, Math.min(6, Number(level)))
    return `\n${'#'.repeat(depth)} ${cleanupInline(inner)}\n\n`
  })

  content = content.replace(/<a[^>]*href="([^"]+)"[^>]*>([\s\S]*?)<\/a>/gi, (_, href, inner) => {
    const text = cleanupInline(inner)
    const absoluteHref = href.startsWith('http') ? href : new URL(href, baseUrl).toString()
    return text ? `[${text}](${absoluteHref})` : absoluteHref
  })

  content = content.replace(/<(strong|b)[^>]*>([\s\S]*?)<\/(strong|b)>/gi, (_, __, inner) => `**${cleanupInline(inner)}**`)
  content = content.replace(/<(em|i)[^>]*>([\s\S]*?)<\/(em|i)>/gi, (_, __, inner) => `*${cleanupInline(inner)}*`)
  content = content.replace(/<li[^>]*>/gi, '- ')
  content = content.replace(/<\/li>/gi, '\n')
  content = content.replace(/<ul[^>]*>|<ol[^>]*>/gi, '\n')
  content = content.replace(/<\/ul>|<\/ol>/gi, '\n')
  content = content.replace(/<tr[^>]*>/gi, '')
  content = content.replace(/<(td|th)[^>]*>/gi, '| ')
  content = content.replace(/<\/(td|th)>/gi, ' ')
  content = content.replace(/<\/tr>/gi, '|\n')
  content = content.replace(/<table[^>]*>/gi, '\n')
  content = content.replace(/<\/table>/gi, '\n')
  content = content.replace(/<br\s*\/?>/gi, '\n')
  content = content.replace(/<\/p>/gi, '\n\n')
  content = content.replace(/<\/div>/gi, '\n')
  content = content.replace(/<[^>]+>/g, ' ')
  content = decodeEntities(content)
  content = content.replace(/[ \t]+\n/g, '\n')
  content = content.replace(/\n[ \t]+/g, '\n')
  content = content.replace(/\n\|\n/g, '\n')
  content = content.replace(/\n{3,}/g, '\n\n')
  content = content.replace(/[ \t]{2,}/g, ' ')

  return content.trim() + '\n'
}

async function fetchText(url) {
  const response = await fetch(url, {
    headers: {
      'User-Agent': 'Mozilla/5.0 (compatible; CodexFetcher/1.0)',
    },
  })
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText} for ${url}`)
  }
  return await response.text()
}

async function fetchBinary(url) {
  const response = await fetch(url, {
    headers: {
      'User-Agent': 'Mozilla/5.0 (compatible; CodexFetcher/1.0)',
    },
  })
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText} for ${url}`)
  }
  return Buffer.from(await response.arrayBuffer())
}

await rm(outDir, { recursive: true, force: true })
await mkdir(pagesDir, { recursive: true })
await mkdir(markdownDir, { recursive: true })
await mkdir(assetsDir, { recursive: true })

const entryHtml = await fetchText(entryUrl)
const pagePaths = [entryPath, ...extractWikiLinks(entryHtml)]
const downloadedAssets = new Map()
const pageRecords = []

for (const pagePath of pagePaths) {
  const pageUrl = new URL(pagePath, baseUrl).toString()
  const html = await fetchText(pageUrl)
  const contentHtml = extractContent(html)
  const imageUrls = extractImageSources(contentHtml)
  const pageAssetMap = new Map()

  for (const imageUrl of imageUrls) {
    let assetName = downloadedAssets.get(imageUrl)
    if (!assetName) {
      assetName = safeAssetName(imageUrl)
      await writeFile(path.join(assetsDir, assetName), await fetchBinary(imageUrl))
      downloadedAssets.set(imageUrl, assetName)
    }
    pageAssetMap.set(imageUrl, assetName)
  }

  const slug = safeSlugFromPath(pagePath)
  const title = extractTitle(html)
  const heading = extractFirstHeading(contentHtml)

  await writeFile(path.join(pagesDir, `${slug}.html`), html, 'utf8')
  await writeFile(path.join(markdownDir, `${slug}.md`), htmlToMarkdown(contentHtml, pageAssetMap), 'utf8')

  pageRecords.push({
    slug,
    title,
    heading,
    source: pageUrl,
    markdown: `markdown/${slug}.md`,
    html: `pages/${slug}.html`,
    images: [...pageAssetMap.values()].map((name) => `assets/${name}`),
  })
}

const fetchedAt = new Date().toISOString()
const readmeLines = [
  '# im-mortal.cn 凡人修仙传文档抓取',
  '',
  `- 来源入口: ${entryUrl}`,
  `- 抓取时间: ${fetchedAt}`,
  '- 说明: 保留原始 HTML 快照，并额外生成便于仓库内阅读和继续整理的 Markdown 版本。',
  '',
  '## 文档索引',
  '',
]

for (const record of pageRecords) {
  const label = record.heading ? `${record.title} / ${record.heading}` : record.title
  readmeLines.push(`- ${label}`)
  readmeLines.push(`  - 源地址: ${record.source}`)
  readmeLines.push(`  - Markdown: ${record.markdown}`)
  readmeLines.push(`  - HTML: ${record.html}`)
  if (record.images.length > 0) {
    readmeLines.push(`  - 图片: ${record.images.join(', ')}`)
  }
}
readmeLines.push('')

await writeFile(path.join(outDir, 'README.md'), readmeLines.join('\n'), 'utf8')
await writeFile(path.join(outDir, 'index.json'), JSON.stringify({ source: entryUrl, fetchedAt, pages: pageRecords }, null, 2), 'utf8')

console.log(
  JSON.stringify(
    {
      outDir,
      pages: pageRecords.length,
      assets: downloadedAssets.size,
    },
    null,
    2,
  ),
)
