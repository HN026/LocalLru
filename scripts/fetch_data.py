import yfinance as yf
import os

symbols = ["AAPL", "MSFT", "GOOG", "TSLA"]

# Go one level up from scripts/ and create data/ at project root
base_path = os.path.join(os.path.dirname(__file__), "..", "data")
os.makedirs(base_path, exist_ok=True)

for sym in symbols:
    data = yf.download(sym, period="5d", interval="1m")
    filepath = os.path.join(base_path, f"{sym}.csv")
    data.to_csv(filepath)
    print(f"Saved {filepath}")
