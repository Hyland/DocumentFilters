using Hyland.DocumentFilters;
using System.Text;

/// <summary>
/// The Redactor class processes and returns the redaction rectangles for a page.
/// </summary>
class Redactor
{
    public string Body { get; private set; }
    public Page Page { get; private set; }
    private readonly List<(int Offset, int WordIndex)> _wordOffsets = new();

    public Redactor(Page page)
    {
        Page = page;
        int offset = 0;
        int lr = 0, lb = 0;

        StringBuilder sb = new();
        for (int i = 0, c = page.WordCount; i < c; ++i)
        {
            Word word = page.GetWord(i);
            if (i != 0)
            {
                if (word.X > lr)
                {
                    _ = sb.Append(' ');
                    offset++;
                }
                else if (word.Y > lb)
                {
                    _ = sb.Append('\n');
                    offset++;
                }
            }

            _wordOffsets.Add((offset, i));
            _ = sb.Append(word.Text);
            offset += word.Text.Length;

            lr = word.X + word.Width;
            lb = word.Y + word.Height;
        }
        Body = sb.ToString();
    }

    public IEnumerable<(System.Drawing.Rectangle Rect, Word Word)> GetRedactions(IEnumerable<IRule> rules)
    {
        int findWordIndex(int offset)
        {
            Comparer<(int, int)> comparer = Comparer<(int, int)>.Create(((int Offset, int WordIndex) left, (int Offset, int WordIndex) right) =>
            {
                return left.Offset - right.Offset;
            });

            int res = _wordOffsets.BinarySearch((offset, -1), comparer);
            if (res < 0)
                return (~res) - 1;
            return res;
        }

        foreach (IRule rule in rules)
        {
            foreach ((int From, int To) match in rule.Match(Body))
            {
                int f = findWordIndex(match.From);
                int t = findWordIndex(match.To);

                int lt = 0, lr = 0;
                for (int i = f; i <= t; ++i)
                {
                    Word w = Page.GetWord(i);
                    (System.Drawing.Rectangle, Word w) res = (new System.Drawing.Rectangle(w.X, w.Y, w.Width, w.Height), w);
                    if (i > f && w.Y == lt && w.X > lr)
                        res.Item1 = System.Drawing.Rectangle.FromLTRB(lr, w.Y, w.X + w.Width, w.Y + w.Height);
                    lr = w.X + w.Width;
                    lt = w.Y;

                    yield return res;
                }
            }
        }
    }

    public static IEnumerable<(System.Drawing.Rectangle Rect, Word Word)> Redact(Page page, IEnumerable<IRule> rules)
    {
        Redactor r = new(page);
        return r.GetRedactions(rules);
    }
}
