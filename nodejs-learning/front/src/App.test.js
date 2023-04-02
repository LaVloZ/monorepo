import { render, screen } from '@testing-library/react';
import { jest } from '@jest/globals'
import App from './App';

describe('product home page', () => {

  let fetch

  beforeEach(async () => {
    fetch = global.fetch
    global.fetch = jest.fn((resource) => {
      if(resource === 'product')
        return Promise.resolve({
          json: Promise.resolve({
            name: 'Salade césarienne'
          })
        })
    
    })
  })

  afterEach(() => {
    global.fetch = fetch
  })

  test('show product title : salade césarienne', async () => {
    render(<App />);
  
    const linkElement = screen.getByText('Salade césarienne')
    expect(linkElement).toBeInTheDocument()
  });
  
  
  test('show product price : 5,00 €', () => {
    render(<App />);
    
    const linkElement = screen.getByText('5,00 €')
    expect(linkElement).toBeInTheDocument()
  });
  
  test('show product picture €', () => {
    render(<App />);
    
    const linkElement = screen.getByAltText('Salade césarienne')
    expect(linkElement).toBeInTheDocument()
  });
})
