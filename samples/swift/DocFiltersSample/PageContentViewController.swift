// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import UIKit

class PageContentViewController: UIViewController {

    @IBOutlet var image: UIImageView!
    @IBOutlet var heading: UILabel!

    var pageIndex: Int?
    var caption: String!
    var pageImage: UIImage?

    override func viewDidLoad() {
        super.viewDidLoad()

        heading.layer.cornerRadius = 5
        heading.text = caption
        image.image = pageImage
        heading.alpha = 0.1

        // Fade in then out
        UIView.animate(withDuration: 1.0, animations: { () -> Void in
            self.heading.alpha = 1.0
        }, completion: { (_) -> Void in
            UIView.animate(withDuration: 5.0, animations: { () -> Void in
                self.heading.alpha = 0.0
            })
        })
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    /*
     // MARK: - Navigation

     // In a storyboard-based application, you will often want to do a little preparation before navigation
     override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
     // Get the new view controller using segue.destinationViewController.
     // Pass the selected object to the new view controller.
     }
     */
}
