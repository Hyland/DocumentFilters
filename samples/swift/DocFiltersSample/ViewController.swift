// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import UIKit

class ViewController: UIViewController, UIPageViewControllerDataSource, UIPageViewControllerDelegate {

    @IBOutlet var activityIndicator: UIActivityIndicatorView!

    @IBOutlet var errorMessage: UILabel!

    var pageViewController: UIPageViewController!
    var docfilters = DocumentFilters.Instance()
    var pagecount: Int = 0
    var document: DocumentFilters.Extractor?

    let dispatchQueue = DispatchQueue(label: "Dispatch Queue", attributes: [], target: nil)

    public var file: String?
    public var subfileContainer: DocumentFilters.Extractor?
    public var subfileId: String?
    public var subfileName: String?

    @IBAction func swipeLeft(sender _: AnyObject) {
        // println("Swipe left")
    }

    @IBAction func swiped(sender _: AnyObject) {

        pageViewController.view.removeFromSuperview()
        pageViewController.removeFromParentViewController()
        reset()
    }

    func reset() {
        var filename: String = ""

        if file != nil {
            filename = Bundle.main.bundlePath + "/sample_docs/" + file!
        } else if subfileName != nil {
            filename = subfileName!
        } else {
            return
        }

        title = (filename as NSString).lastPathComponent

        dispatchQueue.async {
            do {
                try self.docfilters.Initialize(ISYS_LICENSE_KEY)

                if self.file != nil {
                    self.document = try self.docfilters.GetExtractor(filename: filename)
                } else if self.subfileId != nil && self.subfileContainer != nil {
                    self.document = try self.subfileContainer?.GetSubFile(id: self.subfileId!)
                } else {
                    return
                }

                try self.document?.Open(flags: [.kFormatImage], options: "CONTAINERS_LIST_DIR=on")
                self.pagecount = (try self.document?.getPageCount())!

                OperationQueue.main.addOperation {
                    self.activityIndicator.stopAnimating()

                    if (self.document?.SupportsSubFiles)! && !(self.document?.SupportsHD)! {
                        let x = self.storyboard?.instantiateViewController(withIdentifier: "SubFileController") as! SubFileTableViewController
                        x.document = self.document
                        self.addChildViewController(x)
                        self.view.addSubview(x.view)
                        x.didMove(toParentViewController: self)
                    } else {
                        self.pageViewController = self.storyboard?.instantiateViewController(withIdentifier: "PageViewController") as! UIPageViewController
                        self.pageViewController.dataSource = self

                        let pageContentViewController = self.viewControllerAtIndex(index: 0)
                        self.pageViewController.setViewControllers([pageContentViewController!], direction: UIPageViewControllerNavigationDirection.forward, animated: true, completion: nil)
                        self.pageViewController.view.frame = CGRect(x: 0, y: 0, width: self.view.frame.width, height: self.view.frame.size.height)

                        self.addChildViewController(self.pageViewController)
                        self.view.addSubview(self.pageViewController.view)
                        self.pageViewController.didMove(toParentViewController: self)
                    }
                }
            } catch let DocumentFilters.Exception.Unknown(message) {
                self.errorMessage.text = message
                self.errorMessage.isHidden = false
                self.activityIndicator.stopAnimating()
            } catch {
                self.errorMessage.text = "Something went wrong"
                self.errorMessage.isHidden = false
                self.activityIndicator.stopAnimating()
            }
        }
    }

    @IBAction func start(sender _: AnyObject) {
        let pageContentViewController = viewControllerAtIndex(index: 0)
        pageViewController.setViewControllers([pageContentViewController!], direction: UIPageViewControllerNavigationDirection.forward, animated: true, completion: nil)
    }

    func pageViewController(_: UIPageViewController, viewControllerAfter viewController: UIViewController) -> UIViewController? {

        let index = (viewController as! PageContentViewController).pageIndex!

        if index >= getPageCount() {
            return nil
        }
        return viewControllerAtIndex(index: index + 1)
    }

    func pageViewController(_: UIPageViewController, viewControllerBefore viewController: UIViewController) -> UIViewController? {

        let index = (viewController as! PageContentViewController).pageIndex!
        if index <= 0 {
            return nil
        }
        return viewControllerAtIndex(index: index - 1)
    }

    func viewControllerAtIndex(index: Int) -> UIViewController? {
        if index >= getPageCount() {
            return nil
        }
        let pageContentViewController = storyboard?.instantiateViewController(withIdentifier: "PageContentViewController") as! PageContentViewController

        do {
            let page = try document?.GetPage(index)
            pageContentViewController.pageImage = try page?.GetImage()
        } catch {
        }

        pageContentViewController.caption = "Page \(index + 1)"
        pageContentViewController.pageIndex = index
        return pageContentViewController
    }

    func presentationCountForPageViewController(pageViewController _: UIPageViewController) -> Int {
        return getPageCount() - 1
    }

    func presentationIndexForPageViewController(pageViewController _: UIPageViewController) -> Int {
        return 0
    }

    func getPageCount() -> Int {
        return pagecount
    }

    override func viewWillAppear(_: Bool) {
        reset()
    }

    override func viewDidLoad() {
        super.viewDidLoad()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}
