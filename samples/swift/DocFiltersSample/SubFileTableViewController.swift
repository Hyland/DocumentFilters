// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import UIKit

class SubFileTableViewController: UITableViewController {
    struct Entry {
        var id: String
        var filename: String
        var size: Int64
        var date: Date?
    }

    private var files: [Entry] = []

    public var document: DocumentFilters.Extractor?

    let dispatchQueue = DispatchQueue(label: "Dispatch Queue", attributes: [], target: nil)

    override func viewDidLoad() {
        super.viewDidLoad()

        reset()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    func reset() {
        dispatchQueue.async {
            do {
                var subfile = try self.document?.GetFirstSubFile()
                while subfile != nil {
                    self.files.append(Entry(id: (subfile?.ID)!, filename: (subfile?.Name)!, size: (subfile?.FileSize)!, date: subfile?.FileTime))

                    subfile = try self.document?.GetNextSubFile()
                }
                OperationQueue.main.addOperation() {
                    self.tableView.reloadData()
                }
            } catch {
            }
        }
    }

    func dateToString(_ date: Date?) -> String {
        if date == nil {
            return ""
        } else {
            let dateformatter = DateFormatter()

            dateformatter.dateStyle = DateFormatter.Style.short
            dateformatter.timeStyle = DateFormatter.Style.short

            return dateformatter.string(from: date!)
        }
    }

    func sizeToString(_ size: Int64) -> String {
        if size < 1024 {
            return "\(size) bytes"
        } else if size < 1024 * 1024 {
            return "\(size / 1024) KB"
        } else if size < 1024 * 1024 * 1024 {
            return "\(size / (1024 * 1024)) MB"
        } else {
            return "\(size / (1024 * 1024 * 1024)) GB"
        }
    }

    // MARK: - Table view data source

    override func numberOfSections(in _: UITableView) -> Int {
        return 1
    }

    override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
        return files.count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cellIdentifier = "SubFileTableViewCell"

        guard let cell = tableView.dequeueReusableCell(withIdentifier: cellIdentifier, for: indexPath) as? SubFileTableViewCell else {
            fatalError("The dequeued cell is not an instance of \(cellIdentifier).")
        }

        let file = files[indexPath.row]
        cell.labelFilename.text = file.filename
        cell.labelSize.text = sizeToString(file.size)
        cell.labelDate.text = dateToString(file.date)

        return cell
    }

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        super.prepare(for: segue, sender: sender)

        guard let viewController = segue.destination as? ViewController else {
            fatalError("Unexpected destination: \(segue.destination)")
        }

        guard let selectedDirBrowseCell = sender as? SubFileTableViewCell else {
            fatalError("Unexpected sender")
        }

        guard let indexPath = tableView.indexPath(for: selectedDirBrowseCell) else {
            fatalError("The selected cell is not being displayed by the table")
        }

        let selectedFile = files[indexPath.row]
        viewController.subfileContainer = document
        viewController.subfileId = selectedFile.id
        viewController.subfileName = selectedFile.filename
    }
}
