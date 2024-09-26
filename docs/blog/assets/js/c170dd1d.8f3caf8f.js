"use strict";(self.webpackChunkmy_website=self.webpackChunkmy_website||[]).push([[137],{6662:(e,t,n)=>{n.r(t),n.d(t,{assets:()=>m,contentTitle:()=>u,default:()=>g,frontMatter:()=>d,metadata:()=>h,toc:()=>p});var a=n(4848),i=n(8453),r=n(778),s=n(1891),o=n(1912),l=n(1951),c=n(2602);const d={slug:"24.3-release",title:"Document Filters 24.3 Release",tags:["Release"],authors:["nabihmetri"],image:"./Thumbnail.png",description:"Explore the new features of Hyland Document Filters with the 24.3 release, ranging from table detection to a markdown output type.",keywords:["text extraction","document conversion","document transformation","document filters","text extraction","document conversion","document transformation","blog","demo","walk through","release","table detection","json","markdown","msi","hwpx","AI","ML"]},u="Hyland Document Filters 24.3 Release",h={permalink:"/DocumentFilters/blog/24.3-release",source:"@site/blog/2024-08-21-24.3-release/index.md",title:"Document Filters 24.3 Release",description:"Explore the new features of Hyland Document Filters with the 24.3 release, ranging from table detection to a markdown output type.",date:"2024-08-21T00:00:00.000Z",tags:[{label:"Release",permalink:"/DocumentFilters/blog/tags/release"}],readingTime:3.17,hasTruncateMarker:!0,authors:[{name:"Nabih Metri",title:"Document Filters Product Owner",imageURL:"https://github.com/nmetrihyland.png",key:"nabihmetri"}],frontMatter:{slug:"24.3-release",title:"Document Filters 24.3 Release",tags:["Release"],authors:["nabihmetri"],image:"./Thumbnail.png",description:"Explore the new features of Hyland Document Filters with the 24.3 release, ranging from table detection to a markdown output type.",keywords:["text extraction","document conversion","document transformation","document filters","text extraction","document conversion","document transformation","blog","demo","walk through","release","table detection","json","markdown","msi","hwpx","AI","ML"]},unlisted:!1,prevItem:{title:"Shifting Left with Document Filters \u2013 A Vision for the Future",permalink:"/DocumentFilters/blog/shift-left-vision"},nextItem:{title:"Exploring the Document Comparison APIs",permalink:"/DocumentFilters/blog/comparison-api"}},m={image:n(138).A,authorsImageUrls:[void 0]},p=[{value:"PDF Table Extraction",id:"pdf-table-extraction",level:2},{value:"JSON Output Type",id:"json-output-type",level:2},{value:"Markdown Output Type",id:"markdown-output-type",level:2},{value:"Markdown AI Use Case",id:"markdown-ai-use-case",level:3},{value:"MSI Installer Sub-File Extraction",id:"msi-installer-sub-file-extraction",level:2},{value:"Hancom Hangul HWPX Text and HD Support (Community-Inspired)",id:"hancom-hangul-hwpx-text-and-hd-support-community-inspired",level:2},{value:"Release Links",id:"release-links",level:2},{value:"Document Filters Resources",id:"document-filters-resources",level:2}];function f(e){const t={a:"a",h2:"h2",h3:"h3",li:"li",p:"p",ul:"ul",...(0,i.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(t.p,{children:"We're excited to announce the latest release of Document Filters, packed with powerful new features designed to enhance your document processing capabilities. This update introduces a JSON Output Type for structured data handling, a Markdown Output Type for streamlined document conversion, advanced PDF Table Extraction for improved data accuracy, and MSI Installer Sub-File Extraction for comprehensive file analysis. Additionally, we've added community-inspired support for Hancom Hangul HWPX text extraction and HD rendering. Read on to discover how these new features can elevate your workflows and drive better results."}),"\n",(0,a.jsx)(t.p,{children:"Watch as we walk through a few of the new features in the 24.3 release of Document Filters."}),"\n",(0,a.jsx)(l.A,{src:"https://www.youtube.com/embed/Iou3fnOuTSo?si=zSEciPzwpaO4nEBA",alt:"Hyland Document Filters - 24.3 Features Preview Video"}),"\n",(0,a.jsx)(t.h2,{id:"pdf-table-extraction",children:"PDF Table Extraction"}),"\n",(0,a.jsx)(t.p,{children:"Document Filters now supports the identification and extraction of tables from untagged PDF files. This feature preserves the logical structure of tables, rows, and cells, ensuring accurate table detection for PDFs. Preserving the structures of tables allows AI/ML systems to improve their data quality and accuracy when processing documents."}),"\n",(0,a.jsx)(c.A,{src:n(6768).A,alt:"Hyland Document Filters - PDF Table Extraction"}),"\n",(0,a.jsx)(t.p,{children:"Detecting and extracting the table information from an invoice. The image on the left displays how information is extracted with table detection disabled, while the right displays how information is extrated with table detection enabled."}),"\n",(0,a.jsx)(t.h2,{id:"json-output-type",children:"JSON Output Type"}),"\n",(0,a.jsx)(t.p,{children:"Document Filters now includes support for a JSON output type, which structures document data in a detailed, hierarchical format. This enhancement facilitates seamless integration with AI and other JSON-compatible applications, ensuring efficient parsing and utilization of document content for improved AI/ML-driven data analysis and processing."}),"\n",(0,a.jsx)(o.A,{src:r.A,alt:"Hyland Document Filters - JSON Output Type"}),"\n",(0,a.jsx)(t.p,{children:"Converting an invoice with a table into JSON, and keeping its table structure."}),"\n",(0,a.jsx)(t.h2,{id:"markdown-output-type",children:"Markdown Output Type"}),"\n",(0,a.jsx)(t.p,{children:"Converting documents to a Markdown output type is now supported. This feature allows users to effortlessly convert documents to Markdown, providing a lightweight and efficient way to present formatted content. It is ideal for use with AI/ML systems, as the lightweight and structured composition of Markdown reduces computing costs, as well as enhances data processing."}),"\n",(0,a.jsx)(c.A,{src:n(9912).A,alt:"Hyland Document Filters - Markdown Output Type"}),"\n",(0,a.jsx)(t.p,{children:"Converting an invoice with a table into Markdown, and keeping its table structure."}),"\n",(0,a.jsx)(t.h3,{id:"markdown-ai-use-case",children:"Markdown AI Use Case"}),"\n",(0,a.jsx)(o.A,{src:s.A,alt:"Hyland Document Filters - Markdown AI Use Case"}),"\n",(0,a.jsx)(t.p,{children:'Here we passed in the Markdown generated from the invoice into ChatGPT and gave it the prompt of "Complete this invoice and display it in a readable format." ChatGPT was able to correctly identify and understand the context of the table and fill in the missing values. Using Markdown with table detection allowed ChatGPT to have better context compared to plain text, as well as need less resources compared to full JSON, HTML, or XML.'}),"\n",(0,a.jsx)(t.h2,{id:"msi-installer-sub-file-extraction",children:"MSI Installer Sub-File Extraction"}),"\n",(0,a.jsx)(t.p,{children:"Our latest update introduces support for sub-file extraction from MSI installer files. This addition enables better handling and analysis of MSI files by extracting embedded files for further processing, enhancing the overall file analysis workflow in security systems."}),"\n",(0,a.jsx)(t.h2,{id:"hancom-hangul-hwpx-text-and-hd-support-community-inspired",children:"Hancom Hangul HWPX Text and HD Support (Community-Inspired)"}),"\n",(0,a.jsx)(t.p,{children:"In response to community feedback, we have introduced text extraction and HD rendering support for Hancom Hangul HWPX files. This update enhances the ability to process and utilize content from these files, as well as ensures enhanced rendering fidelity of complex Korean language documents, maintaining their original formatting and layout."}),"\n",(0,a.jsx)(t.h2,{id:"release-links",children:"Release Links"}),"\n",(0,a.jsxs)(t.ul,{children:["\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://docs.hyland.com/DocumentFilters/en_US/Print/release_notes/rel_24_3_0_0000.html",children:"Document Filters 24.3 Release Notes"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://github.com/Hyland/DocumentFilters/releases/download/v24.3.0/document-filters-24.3.0.5160-sbom.json",children:"Document Filters 24.3 Software Bill of Materials"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://community.hyland.com/feedback",children:"Enhancement Requests"})}),"\n"]}),"\n",(0,a.jsx)(t.h2,{id:"document-filters-resources",children:"Document Filters Resources"}),"\n",(0,a.jsxs)(t.ul,{children:["\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://github.com/Hyland/DocumentFilters",children:"Document Filters on GitHub"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://www.nuget.org/packages/Hyland.DocumentFilters",children:"Document Filters on NuGet"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://github.com/Hyland/DocumentFilters/tree/master/samples",children:"Document Filters Samples"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://docs.hyland.com/DocumentFilters/en_US/Print/index.html",children:"Document Filters Documentation"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://docs.hyland.com/DocumentFilters/en_US/Print/release_notes/index.html",children:"Document Filters Release Notes"})}),"\n",(0,a.jsx)(t.li,{children:(0,a.jsx)(t.a,{href:"https://www.hyland.com/en/products/document-filters",children:"Hyland"})}),"\n"]})]})}function g(e={}){const{wrapper:t}={...(0,i.R)(),...e.components};return t?(0,a.jsx)(t,{...e,children:(0,a.jsx)(f,{...e})}):f(e)}},1912:(e,t,n)=>{n.d(t,{A:()=>i});n(6540);var a=n(4848);function i(e){let{src:t,alt:n}=e;return(0,a.jsx)("img",{"data-gifffer":t,"data-gifffer-width":"100%","data-gifffer-height":"auto",alt:n})}},2602:(e,t,n)=>{n.d(t,{A:()=>i});n(6540);var a=n(4848);function i(e){let{src:t,alt:n}=e;return(0,a.jsx)("a",{href:t,target:"_blank",rel:"noopener noreferrer",children:(0,a.jsx)("img",{src:t,alt:n})})}},1951:(e,t,n)=>{n.d(t,{A:()=>i});n(6540);var a=n(4848);function i(e){let{src:t,alt:n}=e;return(0,a.jsx)("div",{className:"iframe-container",children:(0,a.jsx)("iframe",{src:t,title:n,enablejsapi:"true",frameBorder:"0",allow:"autoplay; encrypted-media; picture-in-picture; web-share",referrerPolicy:"strict-origin-when-cross-origin",allowFullScreen:!0})})}},138:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/Thumbnail-a85bf36120ca8a0b046fd69d890d87f8.png"},778:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/JSON-047d132fda1eb5fd56168e99237ba81e.gif"},9912:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/Markdown-8a65dc0021a1553bcbc364aa54e8573f.png"},1891:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/Markdown_ChatGPT-07dd432a4198afc67fd0aa16e63d3f9a.gif"},6768:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/PDF_TABLE_DETECTION-d19ee1f757ec4dfe9651f605b62b34f0.png"},8453:(e,t,n)=>{n.d(t,{R:()=>s,x:()=>o});var a=n(6540);const i={},r=a.createContext(i);function s(e){const t=a.useContext(r);return a.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function o(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(i):e.components||i:s(e.components),a.createElement(r.Provider,{value:t},e.children)}}}]);