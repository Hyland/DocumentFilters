// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import UIKit

class DirBrowseTableViewController: UITableViewController, UISearchBarDelegate {

    @IBOutlet var searchBar: UISearchBar!

    private var files: [String] = []
    private var searchResults: [String] = []
    private var searchActive: Bool = false

    private func loadFiles() {
        let dir = Bundle.main.bundlePath + "/sample_docs"
        do {
            files = try FileManager.default.contentsOfDirectory(atPath: dir)
            files.sort()
            searchResults = files
        } catch {
            // TODO:
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        searchBar!.delegate = self

        loadFiles()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    func results() -> [String] {
        return searchResults
    }

    // MARK: - Table view data source

    override func numberOfSections(in _: UITableView) -> Int {
        return 1
    }

    override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
        return results().count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cellIdentifier = "DirBrowseTableViewCell"

        guard let cell = tableView.dequeueReusableCell(withIdentifier: cellIdentifier, for: indexPath) as? DirBrowseTableViewCell else {
            fatalError("The dequeued cell is not an instance of \(cellIdentifier).")
        }

        let file = results()[indexPath.row]
        cell.labelFilename.text = file

        return cell
    }

    func filterContentForSearchText(searchText: String) {
        if files.count == 0 {
            searchResults = []
        } else if searchText.isEmpty {
            searchResults = files
        } else {
            searchResults = files.filter({ (item) -> Bool in
                ((item.lowercased().range(of: searchText.lowercased())?.lowerBound) != nil)
            })
        }
    }

    func resetSearch() {
        searchActive = false
        searchBar.text = ""
        searchResults = files
    }

    func searchBarTextDidBeginEditing(_: UISearchBar) {
        searchActive = true
    }

    func searchBarTextDidEndEditing(_: UISearchBar) {
        searchActive = false
    }

    func searchBarCancelButtonClicked(_: UISearchBar) {
        resetSearch()
        tableView.reloadData()
    }

    func searchBarSearchButtonClicked(_: UISearchBar) {
        searchActive = false
    }

    func searchBar(_: UISearchBar, textDidChange searchText: String) {
        filterContentForSearchText(searchText: searchText)
        tableView.reloadData()
    }

    /*
     // Override to support conditional editing of the table view.
     override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
     // Return false if you do not want the specified item to be editable.
     return true
     }
     */

    /*
     // Override to support editing the table view.
     override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
     if editingStyle == .delete {
     // Delete the row from the data source
     tableView.deleteRows(at: [indexPath], with: .fade)
     } else if editingStyle == .insert {
     // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
     }
     }
     */

    /*
     // Override to support rearranging the table view.
     override func tableView(_ tableView: UITableView, moveRowAt fromIndexPath: IndexPath, to: IndexPath) {

     }
     */

    /*
     // Override to support conditional rearranging of the table view.
     override func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
     // Return false if you do not want the item to be re-orderable.
     return true
     }
     */

    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        super.prepare(for: segue, sender: sender)

        guard let viewController = segue.destination as? ViewController else {
            fatalError("Unexpected destination: \(segue.destination)")
        }

        guard let selectedDirBrowseCell = sender as? DirBrowseTableViewCell else {
            fatalError("Unexpected sender")
        }

        guard let indexPath = tableView.indexPath(for: selectedDirBrowseCell) else {
            fatalError("The selected cell is not being displayed by the table")
        }

        let selectedFile = files[indexPath.row]
        viewController.file = selectedFile
    }
}
