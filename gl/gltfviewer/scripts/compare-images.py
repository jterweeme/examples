# Create a virtual env with `python -m venv .venv`
# Activate it with `source .venv/bin/activate` (`source .venv/scripts/activate` on windows)
# Install dependencies with `pip install imutils opencv-python`
# Then run de script, change reference_path and test_path at lines 12 and 14

from pathlib import Path

import cv2
import imutils
import numpy as np

reference_path = Path(".local/output-images/2023-01-05-11-56-39")
test_path = Path(".local/output-images/2023-01-05-12-29-04")
output_path = Path(".local/output-images/_test-output")
output_path.mkdir(exist_ok=True)

for reference_filepath in reference_path.iterdir():
    reference_img = cv2.imread(str(reference_filepath))
    test_filepath = test_path / reference_filepath.name
    test_img = cv2.imread(str(test_filepath))
    diff = reference_img.copy()
    cv2.absdiff(reference_img, test_img, diff)
    gray = cv2.cvtColor(diff, cv2.COLOR_BGR2GRAY)
    if gray.sum() == 0:
        continue
    for i in range(0, 3):
        dilated = cv2.dilate(gray.copy(), None, iterations=i+1)
    (T, thresh) = cv2.threshold(dilated, 3, 255, cv2.THRESH_BINARY)
    if np.count_nonzero(thresh) == 0:
        print(f"{reference_filepath.name} has errors but negligible visually")
        continue
    cv2.imwrite(str(output_path / f"{reference_filepath.stem}-diff.png"), thresh)
    cnts = cv2.findContours(thresh, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
    cnts = imutils.grab_contours(cnts)
    for c in cnts:
        (x, y, w, h) = cv2.boundingRect(c)
        cv2.rectangle(test_img, (x, y), (x + w, y + h), (0, 255, 0), 2)
    cv2.imwrite(str(output_path / f"{reference_filepath.stem}-box.png"), test_img)
