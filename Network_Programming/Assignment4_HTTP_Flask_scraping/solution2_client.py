import requests
import sys

# Flask app URL — must match solution2_app.py
BASE_URL = 'http://127.0.0.1:5000'

sys.stdout.reconfigure(encoding='utf-8')


# TODO: Implement the client script.
#
# - Prompt the user for a search term (keyword, author, etc.).
# - Send a GET request to the /search endpoint.
# - Display the JSON results.
# - Allow repeated searches until the user types "quit".


def search(query: str) -> None:
    """
    Send one GET /search request to the Flask app and pretty-print the results.

    Args:
        query: The search term to send as the 'q' query parameter.
    """
    try:
        resp = requests.get(f"{BASE_URL}/search", params={'q': query}, timeout=10)
        resp.raise_for_status()
        data = resp.json()

        if isinstance(data, list):
            # A list means we got actual publication records.
            print(f"\nFound {len(data)} result(s):\n")
            for i, pub in enumerate(data, start=1):
                print(f"[{i}]")
                for field, value in pub.items():
                    print(f"  {field:<15}: {value}")
                print()
        else:
            # A dict signals a message (no results) or an error from the server.
            print(f"\n{data}\n")

    except requests.exceptions.ConnectionError:
        print(f"Cannot connect to Flask app at {BASE_URL}. Is solution2_app.py running?")
    except requests.exceptions.Timeout:
        print("Request timed out after 10 s.")
    except requests.exceptions.HTTPError as e:
        print(f"HTTP error: {e}")
    except requests.exceptions.RequestException as e:
        print(f"Unexpected request error: {e}")


def main() -> None:
    print(f"DILab Publication Search Client - {BASE_URL}")
    print("Enter a search term, or 'quit' to exit.\n")

    while True:
        try:
            term = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if not term:
            continue

        if term.lower() == 'quit':
            print("Exiting.")
            break

        search(term)


if __name__ == "__main__":
    main()
