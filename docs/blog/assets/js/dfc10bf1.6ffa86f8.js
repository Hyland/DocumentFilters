"use strict";(self.webpackChunkmy_website=self.webpackChunkmy_website||[]).push([[688],{2741:(e,t,n)=>{n.r(t),n.d(t,{assets:()=>l,contentTitle:()=>s,default:()=>h,frontMatter:()=>i,metadata:()=>r,toc:()=>c});var a=n(4848),o=n(8453);const i={slug:"extracting-text-from-any-file-is-harder-than-it-looks",title:"Extracting text from any file is harder than it looks. Extracting formatting is even harder.",tags:[],authors:["bentruscott","coreykidd"],image:"./Thumbnail.png"},s="Extracting text from any file is harder than it looks. Extracting formatting is even harder",r={permalink:"/DocumentFilters/blog/extracting-text-from-any-file-is-harder-than-it-looks",source:"@site/blog/2021-10-04-extracting-text/index.md",title:"Extracting text from any file is harder than it looks. Extracting formatting is even harder.",description:"Backdrop",date:"2021-10-04T00:00:00.000Z",tags:[],readingTime:12.685,hasTruncateMarker:!0,authors:[{name:"Ben Truscott",title:"Document Filters Principal Engineer",imageURL:"https://github.com/bentruscott.png",key:"bentruscott"},{name:"Corey Kidd",title:"(Frm) Product Owner",key:"coreykidd"}],frontMatter:{slug:"extracting-text-from-any-file-is-harder-than-it-looks",title:"Extracting text from any file is harder than it looks. Extracting formatting is even harder.",tags:[],authors:["bentruscott","coreykidd"],image:"./Thumbnail.png"},unlisted:!1,prevItem:{title:"Document Filters 24.1 Release",permalink:"/DocumentFilters/blog/24.1-release"}},l={image:n(2095).A,authorsImageUrls:[void 0,void 0]},c=[{value:"On any platform",id:"on-any-platform",level:2},{value:"Search and extract text at scale",id:"search-and-extract-text-at-scale",level:2},{value:"Make it look good",id:"make-it-look-good",level:2},{value:"Don\u2019t forget security",id:"dont-forget-security",level:2},{value:"Conclusion",id:"conclusion",level:2}];function d(e){const t={a:"a",h2:"h2",img:"img",p:"p",...(0,o.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{alt:"Backdrop",src:n(3349).A+"",width:"1200",height:"630"})}),"\n",(0,a.jsxs)(t.p,{children:["This post was originally hosted on the\n",(0,a.jsx)(t.a,{href:"https://stackoverflow.blog/2021/10/04/extracting-text-from-any-file-is-harder-than-it-looks-extracting-formatting-is-even-harder/",children:"Stack\nOverflow Blog"}),"."]}),"\n",(0,a.jsx)(t.p,{children:"We take for granted document processing on an individual scale: double-click the\nfile (or use a simple command-line phrase) and the contents of the file display.\nBut it gets more complicated at scale. Imagine you\u2019re a recruiter searching\nresumes for keywords or a paralegal looking for names in thousands of pages of\ndiscovery documents. The formats, versions, and platforms that generated them\ncould be wildly different. The challenge is even greater when it\u2019s time\nsensitive, for example if you have to scan all outgoing emails for personally\nidentifiable information (PII) leakages, or you have to give patients a single\nfile that contains all of their disclosure agreements, scanned documents, and\nMRI/X-ray/test reports, regardless of the original file format."}),"\n",(0,a.jsx)(t.p,{children:"At Hyland we produce a document processing toolkit that independent software\nvendors can implement to identify files, extract text, render file content,\nconvert formats, and annotate documents in over 550 formats. These are Document\nFilters, and any software that interacts with documents will need Document\nFilters."}),"\n",(0,a.jsx)(t.p,{children:"One library for 550 formats may seem like overkill, but imagine stringing\ntogether dozens of open source libraries, testing each of these libraries each\ntime a new release hits the wild. We give you one dependency, one point of\ncontact if something goes wrong, and one library to deploy instead of dozens."}),"\n",(0,a.jsx)(t.p,{children:"We started as a company that sold desktop search software called ISYS. The\napplication was built in Pascal for MS-DOS and provided mainframe-level search\non PCs. Eventually, other companies, such as Microsoft and Google, started\nproviding desktop search applications for free, and it\u2019s tough to compete with\nfree."}),"\n",(0,a.jsx)(t.p,{children:"This led us to realize that the sum of the parts was greater than the whole;\ngetting text out of files and delivering the exact location is harder than it\nseems and relevant to applications other than search. Our customers noticed our\nstrength in text extraction and wanted that as something they could integrate or\nembed in their software and across multiple platforms."}),"\n",(0,a.jsx)(t.p,{children:"Identifying that Pascal was not going to meet our needs we pivoted our engineers\nto rebuild the app in C++ over the next year for about half a dozen computing\nplatforms. Since then, we\u2019ve learned a lot about content processing at scale and\nhow to make it work on any platform."}),"\n",(0,a.jsx)(t.h2,{id:"on-any-platform",children:"On any platform"}),"\n",(0,a.jsx)(t.p,{children:"When we rewrote our software, one of the key factors was platform support. At\nthe time, Pascal only supported Windows, and while it now supports Mac and\nLinux, it was and still is a niche language. That wasn\u2019t going to work for us,\nas we wanted to support big backend processing servers like Solaris and HP-UX.\nWe considered writing it in C, but we would have had to invent a lot of the\nboilerplate that C++ gave us for free."}),"\n",(0,a.jsx)(t.p,{children:"We were able to port about 80% of the code from Pascal. The other 20% was new OS\nabstractions, primarily to support the Windows API functions we lose on other\nplatforms and the various quirks of each platform. Each compiler makes different\nassumptions of how to implement C++ code, so we use multiple compilers to see\nwhat those assumptions are."}),"\n",(0,a.jsx)(t.p,{children:"Complicating things was that we not only had to consider operating systems, but\nCPUs as well. Different CPUs process bytes in different orders, called byte\nendianness. All Intel and ARM chips use little-endian, where the least\nsignificant byte is stored first. SPARC chips historically used in Solaris\nmachines used big-endian storage, where the most significant byte was stored\nfirst. When you\u2019re reading from a file, you need to know what chipset produced\nit, otherwise you could read things backwards. We make sure to abstract this\naway so no one needs to figure out the originating chipset before processing a\nfile."}),"\n",(0,a.jsx)(t.p,{children:"Ultimately, the goal is to have the software run exactly the same on all 27\nplatforms. Some of the solution to that problem is just writing everything as\ngenerically as possible without special code for each platform. The other\nsolution is testing. With the conversion to C++, we wrote a lot of new tests in\norder to exercise as much code on all platforms. Today, we\u2019ve expanded those\ntests and made error detection much more strict. Lots of files and formats pass\nthrough during tests and they need to come through clean."}),"\n",(0,a.jsx)(t.h2,{id:"search-and-extract-text-at-scale",children:"Search and extract text at scale"}),"\n",(0,a.jsx)(t.p,{children:"The first step to locating or extracting text from a file is finding out what\nformat the file is in. If you are lucky to get a plaintext file, then that\u2019s an\neasy one. Unfortunately, things are rarely easy. There aren\u2019t a lot of standards\navailable for how files are structured; what exists may be incomplete or\noutdated. Things have changed a lot over the years; Microsoft is actually at the\nforefront for publishing standards. They publish standards for most of their\nfile types these days, particularly the newer ones."}),"\n",(0,a.jsx)(t.p,{children:"Many file types can be identified by an initial set of four bytes. Once you have\nthat, you can quickly parse the file. Older MS Office files all had the same\nfour bytes, which presented complications, especially since so many files were\nin one of the four Office formats. You had to do a little extra detective work.\nNewer Office files all identify as ZIP files\u2014they are all compressed XML\u2014so once\nyou extract the XML, you start applying known heuristics and following markers.\nMost XML is self-describing, so those markers can be easy to follow. Others\ndon\u2019t have much of a path at all."}),"\n",(0,a.jsx)(t.p,{children:"Binary file types are harder. Some of the work here is reverse engineering and\nmaking sure you basically have enough files that are a representative sample\nset. But once you know the pattern, then detecting the file is absolutely\npredictable. We don\u2019t use any machine learning or AI techniques to identify\nfiles because of this. The challenge is working out what the pattern is and what\npattern a given file fits."}),"\n",(0,a.jsx)(t.p,{children:"Identifying files is the very first thing that we do, so it has to be fast. One\nslow detection can impact everything and take us from sub-milliseconds per\ndocument to 15 milliseconds per document. When you're trying to crank through\nforty-thousand documents in a minute, that's a lot."}),"\n",(0,a.jsx)(t.p,{children:"We gain a lot of speed from specializing in text search and extraction as a pure\nback-end system. Alternate methods have used something like LibreOffice to\nprocess documents as a headless word processor. End-user applications have\ngraphic elements and other features that you don\u2019t care about. In a high-traffic\nenvironment, that could mean 50 copies of LibreOffice running as separate\nprocesses across multiple machines, each eating up hundreds of MB. If that\ncrashes it could bring down vital business processes with it. It\u2019s not uncommon\nto see server farms running LibreOffice for conversions that could be replaced\nwith a single back-end process such as Document Filters. That\u2019s before\nconsidering the other workarounds to process all the other file types you might\nneed such as spreadsheets, images, and PDFs."}),"\n",(0,a.jsx)(t.p,{children:"By focusing on processing text at a high volume, we can help clients that need\nto process emails, incoming and outgoing, looking for data loss and accidental\nPII leakages. These products need to scan everything going in or out. We call it\ndeep inspection. We cracked apart every level of an email that could have text.\nZipping something and renaming the extension is not enough to try and trick it.\nAttaching a PDF inside a Word document inside an Excel document is also not\nenough. These are all files that contain text, and security needs to scan all of\nit without delaying the send. We won\u2019t try to crack an encrypted file, but we\ncan flag it for human review. All this is done so quickly that you won\u2019t notice\na delay in the delivery of critical email."}),"\n",(0,a.jsx)(t.p,{children:"We can process text so quickly because we built in C++ and run natively on the\nhardware; targeting native binaries also gives us the greatest flexibility where\nwe can be embedded in applications written in a wide variety of languages. On\ntop of that, all that work identifying file formats pays off. When scanning a\nfile, we load as little as possible into memory, just enough to identify the\nformat. Then we move to processing, where we ignore any information we don\u2019t\nneed to spot text\u2014we don\u2019t need to load Acrobat forms and crack that stuff\napart. Plus we let you throw as much hardware at the problem as you have. Say\nyou are running a POWER8 machine with 200 cores, you can run 400 threads and it\nwon't break a sweat. You want a lot of memory if you're doing that amount of\ndocuments in parallel."}),"\n",(0,a.jsx)(t.h2,{id:"make-it-look-good",children:"Make it look good"}),"\n",(0,a.jsx)(t.p,{children:"Our clients weren\u2019t content with just searching and extracting text; they also\nwanted to display it in web browsers. Around 2009, people wanted to convert\ndocuments to HTML. When extracting text, the software doesn\u2019t care about whether\nsomething is bolded or paginated\u2014we just want the text."}),"\n",(0,a.jsx)(t.p,{children:"Fortunately, all that work we did in understanding file types paid off here. We\nknew how to spot text, the markers that indicated each type, but now we had to\nunderstand the full file structure. Suddenly, bold, italics, tables, page\nbreaks, and tabs vs. spaces become a lot more important. Our first iteration of\nHTML rendering, now called Classic HTML, created an unpaginated free flowing\nversion of the file with as much formatting as we could pull. If you\u2019ve ever\nlooked at the HTML generated by MS Word, you know that creating HTML that\naccurately reflects a document is complicated."}),"\n",(0,a.jsx)(t.p,{children:"There\u2019s seven billion people on the planet and all of them create a Word\ndocument differently. Even within Word or open source .docx editors like\nOpenOffice, you move an element and suddenly the formatting disappears. We had\nto test out all of the possible behaviors in the specifications, and still we\nfigured out some bugs by trial and error."}),"\n",(0,a.jsx)(t.p,{children:"We had one bug where Windows and Mac versions were producing different shades of\nblue. It was consistent across Office documents\u2014PowerPoint and Excel documents\nall showed the same two shades of blue. Sometimes it comes down to different\nsystem defaults and fonts on different platforms. Sometimes the answer is\ncompletely subjective as to what the definition of blue is or whether a line\nwraps before or after a word. In cases like that, you have to pick one of the\ncases to propagate; one of them is right, but it\u2019s hard to suss out exactly\nwhich one. There\u2019s no absolutes."}),"\n",(0,a.jsx)(t.p,{children:"File format specifications, typically published by the vendor, don\u2019t always help\nhere either. We\u2019ve seen a property change, while the spec doesn\u2019t clarify how\nthat affects the formatting of the document. Then, when testing a thousand page\ndocument, we find a bug on page 342, and our collective hearts sink a little\nbit. In cases like these, we know it\u2019s going to take a while to sort out what\u2019s\ncausing it, then prove it over millions of iterations."}),"\n",(0,a.jsx)(t.p,{children:"For all the trouble that Word documents give us, at least there\u2019s structure; you\nknow a table is a table. PDFs have none of that. They are probably the hardest\nto deal with because they focus on how a document is drawn on a screen.\nTechnically, characters can be placed individually anywhere on a page, so\ndetermining column breaks, tables, and other formatting features requires\nlooking at their rendered position on a screen."}),"\n",(0,a.jsx)(t.p,{children:"Pre-internet, everyone had to create everything themselves. They made their own\nformats in the dark. Everyone wrote binaries differently. And PDFs, while they\nare getting better, can always reveal a new bug, no matter how large a corpus of\ntest data we have."}),"\n",(0,a.jsx)(t.p,{children:"Open source software and an increased focus on accessibility concerns have\nchanged formats a lot. PDFs have started including more formatting information\nto accommodate screen readers. Open source software needs to understand file\nformats, so more information is published and file producers have started making\ntheir files easier to understand."}),"\n",(0,a.jsx)(t.p,{children:"The next step after understanding document format was to be able to take these\nfiles and produce paginated output that looks near-pixel perfect to the source\napplication. All that information we learned about file formats, let us create\nwhat we call Paginated HD Renditions. Paginated output means the output looks\nsimilar to if you were to print the document. That\u2019s reading and extracting text\nfrom 550 formats, and creating fully formatted and paginated HD Renditions for\nover 100 formats. Combined with a full markup and annotation API that can create\nnative annotations and export to one of over 20 formats."}),"\n",(0,a.jsx)(t.p,{children:"We\u2019ve talked a lot about Word and PDF documents, because that\u2019s what most people\nuse. But we also can read in exotic file formats, like MRI and CT scan files.\nThis has a significant application in medical situations where you may want to\nconcatenate them with other medical forms, then output a PDF complete with the\ndoctor\u2019s annotations. Want to throw us multiple documents from different file\nformats? Go ahead, we\u2019re not limited to 1:1 input to output, we will ingest the\ndata, understand it, and return it as a single file type of your choice."}),"\n",(0,a.jsx)(t.h2,{id:"dont-forget-security",children:"Don\u2019t forget security"}),"\n",(0,a.jsx)(t.p,{children:"As we moved our product from a desktop search application, we\u2019ve had to increase\nour focus on security. If a consumer-grade product crashes it impacts a single\nuser. But if an embedded piece of software crashes, it could take the rest of\nthe program\u2014possibly the entire server\u2014down with it. These crashes and exploits\ncould open them up for further mischief. Over the years, we did get hit with a\nfew surprises and got burnt."}),"\n",(0,a.jsx)(t.p,{children:"What may be common today certainly wasn\u2019t in the early 2000s. Static analysis,\nunit tests with high code coverage, compiler sanitizers, CVE scans, and\nfuzz-testing are all must haves."}),"\n",(0,a.jsx)(t.p,{children:"We process files of unknown origins and quality. These files might come from a\nthird-party that doesn\u2019t strictly follow specifications, so they might be\ncorrupt, or they might be maliciously crafted to trigger vulnerabilities."}),"\n",(0,a.jsx)(t.p,{children:"Strict adherence to coding and security best practices only gets you so far.\nTesting, both active and passive, is a constantly running background task that\nhelps us in our efforts to detect and gracefully handle the unexpected."}),"\n",(0,a.jsx)(t.p,{children:"Each release is verified with 100K+ files to ensure no regressions or\nperformance degradations. Each nightly build runs over 40K unit tests.\nFuzz-tests number in the 10s of millions. And of course, third-party libraries\nare scanned for vulnerabilities nightly."}),"\n",(0,a.jsx)(t.h2,{id:"conclusion",children:"Conclusion"}),"\n",(0,a.jsx)(t.p,{children:"We\u2019ve lived and breathed file types for decades, and seen the complexities that\ngo into simply finding and extracting text. Some of the largest software\ncompanies in the world leverage Document Filters for their document processing\nneeds, processing terabytes of information hourly. Our team of engineers is\nalways monitoring new and changing file-types so consumers of Document Filters\nare well prepared for the future."}),"\n",(0,a.jsx)(t.p,{children:"If you\u2019re starting a new project, feel there\u2019s room for improvement with your\ncurrent tools, or not wanting to worry about the complexities of document\nprocessing, you can always learn more by checking out our code samples or\nrequesting an evaluation at DocumentFilters.com."})]})}function h(e={}){const{wrapper:t}={...(0,o.R)(),...e.components};return t?(0,a.jsx)(t,{...e,children:(0,a.jsx)(d,{...e})}):d(e)}},2095:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/Thumbnail-3e08c2f527c701572d587afa2d0e7ec6.png"},3349:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/Splash-746084355fbf90b668ce3c8cc955bdbf.png"},8453:(e,t,n)=>{n.d(t,{R:()=>s,x:()=>r});var a=n(6540);const o={},i=a.createContext(o);function s(e){const t=a.useContext(i);return a.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function r(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(o):e.components||o:s(e.components),a.createElement(i.Provider,{value:t},e.children)}}}]);