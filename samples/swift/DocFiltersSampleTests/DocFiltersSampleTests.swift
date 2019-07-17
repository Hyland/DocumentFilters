// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import XCTest
@testable import DocFiltersSample

class DocFiltersSampleTests: XCTestCase {
    
    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }
	
	func testBadInit() {
        let api = DocumentFilters.Instance()
		XCTAssertThrowsError(try api.Initialize("NO LICENSE"))
	}
	func testOpenStream() throws {
		let filename = Bundle.main.bundlePath + "/sample_docs/Sample Envelope.doc"
		let data = try Data(contentsOf: URL(fileURLWithPath: filename))
		
        let api = try DocumentFilters.Instance(ISYS_LICENSE_KEY)
        
        let extractor = try api.GetExtractor(data: data)
        
        let text = try extractor.GetText(1024)
        XCTAssert((text.lowercased().range(of: "street address")?.lowerBound) != nil)
	}
    func testSubFiles() throws {
        let filename = Bundle.main.bundlePath + "/sample_docs/Lexmark Logos.rar"
        
        let api = try DocumentFilters.Instance(ISYS_LICENSE_KEY)
        
        let extractor = try api.GetExtractor(filename: filename)
        
        var subfile: DocumentFilters.SubFile? = try extractor.GetFirstSubFile()
        var count: Int = 0
        while subfile != nil {
            count = count + 1
            XCTAssert(subfile?.Name.range(of: "lxk")?.lowerBound != nil)
            
            let type = try subfile?.getFileType()
            XCTAssert(type == 121 || type == 122 || type == 73 || type == 118)
           
            let calendar = Calendar.current
            let date = subfile?.getFileTime()

            XCTAssertEqual(calendar.component(.year, from: date!), 2014)
            XCTAssertEqual(calendar.component(.month, from: date!), 11)
            
            subfile = try extractor.GetNextSubFile()
        }
        XCTAssertEqual(count, 4)
    }
}
