using System.Text.RegularExpressions;

internal class CreditCardRule : RegExRule
{
    public CreditCardRule()
        : base("(?:\\d[ -]*?){13,16}")
    {
    }
    public override bool IsMatch(Match match) => IsValidCreditCard(match.Value);

    public static bool IsValidCreditCard(string value)
    {
        if (string.IsNullOrWhiteSpace(value) || value.Length == 0)
            return false;

        int endOfString = value.Length;
        while (endOfString > 0 && (value[endOfString - 1] < '0' || value[endOfString - 1] > '9'))
            endOfString--;

        bool t = false;
        int v, sum = 0;
        for (int i = endOfString - 2; i >= 0; i--)
        {
            if (value[i] < '0' || value[i] > '9')
                continue;

            v = value[i] - '0';
            if (t)
                sum += v;
            else
            {
                if (v >= 5)
                    sum += 1 + (v * 2) - 10;
                else
                    sum += v * 2;
            }

            t = !t;
        }
        int checkDigit = 10 - (sum % 10);
        return (value[endOfString - 1] - '0') == checkDigit;
    }
}
