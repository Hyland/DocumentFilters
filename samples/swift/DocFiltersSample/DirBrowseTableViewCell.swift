// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import UIKit

class DirBrowseTableViewCell: UITableViewCell {

    @IBOutlet var labelSize: UILabel!
    @IBOutlet var labelFilename: UILabel!
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }
}
