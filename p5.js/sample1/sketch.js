function setup() {
  createCanvas(600, 400);
}

function draw() {
  background(245);

  stroke(210);
  strokeWeight(2);
  line(150, 0, 150, 400);
  line(300, 0, 300, 400);
  line(450, 0, 450, 400);
  line(0, 133, 600, 133);
  line(0, 266, 600, 266);

  noStroke();

  fill(139, 87, 66);
  ellipse(300, 360, 300, 180);
  rect(260, 240, 80, 60);

  fill(102, 186, 102);
  ellipse(300, 130, 130, 90);

  fill(139, 87, 66);
  ellipse(300, 180, 120, 140);

  fill(186, 135, 112);
  rect(230, 160, 80, 90);
  rect(295, 170, 15, 50);
  rect(275, 145, 15, 60);
  rect(250, 130, 15, 65);

  fill(240, 230, 220);
  rect(225, 140, 20, 70);

  fill(100, 50, 30, 150);
  triangle(230, 250, 320, 250, 275, 180);
}