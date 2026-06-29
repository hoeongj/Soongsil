let startC, endC;

function setup() {
  createCanvas(600, 400);
  colorMode(HSB, 360, 100, 100, 1);
  startC = color(30, 70, 70, 0.6);
  endC = color(210, 70, 70, 0.6);
}

function draw() {
  background(245);

  colorMode(RGB, 255);
  stroke(210);
  strokeWeight(2);
  line(150, 0, 150, 400);
  line(300, 0, 300, 400);
  line(450, 0, 450, 400);
  line(0, 133, 600, 133);
  line(0, 266, 600, 266);
  colorMode(HSB, 360, 100, 100, 1);

  noStroke();

  colorMode(RGB, 255);
  fill(139, 87, 66);
  ellipse(300, 360, 300, 180);
  rect(260, 240, 80, 60);
  colorMode(HSB, 360, 100, 100, 1);

  let angle = frameCount * (TWO_PI / 120);

  push();

  let swayX = sin(angle) * 50;
  translate(swayX, 0);

  noStroke();
  let rainbowHue = map(angle, 0, TWO_PI, 0, 360) % 360;
  let pulse = sin(angle) * 40;
  fill(rainbowHue, 100, 100, 0.2);
  ellipse(300, 130, 130 + pulse + 30, 90 + pulse + 30);
  fill(rainbowHue, 100, 100, 0.4);
  ellipse(300, 130, 130 + pulse + 15, 90 + pulse + 15);
  fill(rainbowHue, 100, 100, 0.6);
  ellipse(300, 130, 130 + pulse, 90 + pulse);

  colorMode(RGB, 255);
  fill(139, 87, 66);
  ellipse(300, 180, 120, 140);
  colorMode(HSB, 360, 100, 100, 1);

  colorMode(RGB, 255);
  fill(186, 135, 112);
  rect(230, 160, 80, 90);
  rect(295, 170, 15, 50);
  rect(275, 145, 15, 60);
  rect(250, 130, 15, 65);

  fill(240, 230, 220);
  rect(225, 140, 20, 70);
  colorMode(HSB, 360, 100, 100, 1);

  let colorAmt = (sin(angle) + 1) / 2;
  fill(lerpColor(startC, endC, colorAmt));

  let shiftX = cos(angle) * 30;
  triangle(230 + shiftX, 250, 320 + shiftX, 250, 275 + shiftX, 180);

  pop();
}

function keyPressed() {
  if (key === 's' || key === 'S') {
    saveGif('assignment4_sway_and_rainbow', 4);
  }
}