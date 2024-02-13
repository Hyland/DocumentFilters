using System.Text.RegularExpressions;

public class RegExRule : IRule
{
    private readonly Regex _re;

    public RegExRule(string expr, RegexOptions opts = RegexOptions.IgnoreCase | RegexOptions.Compiled)
    {
        _re = new Regex(expr, opts);
    }

    public IEnumerable<(int From, int To)> Match(string body)
    {
        MatchCollection matches = _re.Matches(body);
        foreach (Match match in matches)
        {
            if (IsMatch(match))
            {
                if (match.Groups.ContainsKey("redact"))
                {
                    Group group = match.Groups["redact"];
                    yield return (group.Index, group.Index + group.Length);
                }
                else
                {
                    yield return (match.Index, match.Index + match.Length);
                }
            }
        }
    }

    public virtual bool IsMatch(Match match) => true;
}
