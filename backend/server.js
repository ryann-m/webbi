const express = require('express');

const app = express();

app.get('/counter', (req, res) => {

  // Example number
  // Replace later with real analytics

  res.json({
    count: 5600
  });
});

app.listen(3000, () => {

  console.log('Server running on port 3000');
});